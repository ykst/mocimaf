#include "event_logger_driver_impl.h"
#include "ring_logger.h"

typedef struct _record {
    scanline_t scanline;
    event_id_t event_id;
    bool is_trace;
    uint16_t pc;
    union {
        event_domain_id_t domain_id;
        interruption_type_t int_type;
        struct {
            ppu_reg_addr_t reg;
            uint8_t value;
            bool is_read;
        } ppu_reg;
        uint16_t dma_addr;
    };
} _record_t;

typedef struct event_logger_driver {
    bool trace_enabled;
    core_ref shared_core;
    ring_logger_ref rlogger;
} event_logger_driver_t;

void event_logger_driver_destroy(event_logger_driver_ref self)
{
    if (self) {
        ring_logger_destroy(self->rlogger);
        FREE(self);
    }
}

event_logger_driver_ref event_logger_driver_create(core_ref core)
{
    event_logger_driver_ref self = NULL;

    TALLOC(self);
    GUARD(self->shared_core = core);
    GUARD(self->rlogger = ring_logger_create(1024, sizeof(_record_t)));

    return self;
error:
    event_logger_driver_destroy(self);
    return NULL;
}

void event_logger_driver_on_cpu_decode(event_logger_driver_ref self,
                                       cpu_ref cpu, uint16_t pc)
{
}

void event_logger_driver_on_cpu_read(event_logger_driver_ref self, cpu_ref cpu,
                                     uint16_t addr, size_t size)
{
}

void event_logger_driver_on_cpu_write(event_logger_driver_ref self, cpu_ref cpu,
                                      uint16_t addr, size_t length)
{
}

static inline void _logger_append(event_logger_driver_ref self, _record_t *log)
{
    log->scanline = self->shared_core->ppu->scanline;
    log->pc = self->shared_core->cpu->reg.pc;
    ring_logger_append(self->rlogger, log);
}

void event_logger_driver_on_ppu_sprite0_hit(event_logger_driver_ref self,
                                            ppu_ref ppu)
{
    _logger_append(self, &(_record_t){.event_id = EVENT_ID_PPU_SPRITE0_HIT});
}

void event_logger_driver_on_ppu_vblank(event_logger_driver_ref self,
                                       ppu_ref ppu)
{
    _logger_append(self, &(_record_t){.event_id = EVENT_ID_PPU_VBLANK});
}

void event_logger_driver_on_ppu_reg(event_logger_driver_ref self, ppu_ref ppu,
                                    bool is_read, uint8_t value,
                                    ppu_reg_addr_t target)
{
    _logger_append(self, &(_record_t){.event_id = EVENT_ID_PPU_REG,
                                      .ppu_reg = {.is_read = is_read,
                                                  .reg = target,
                                                  .value = value}});
}

void event_logger_driver_on_bus_int_asserted(event_logger_driver_ref self,
                                             interruption_type_t target)
{
    _logger_append(self, &(_record_t){.event_id = EVENT_ID_BUS_INT_ASSERTED,
                                      .int_type = target});
}

void event_logger_driver_on_bus_int_acked(event_logger_driver_ref self,
                                          interruption_type_t target)
{
    _logger_append(self, &(_record_t){.event_id = EVENT_ID_BUS_INT_ACKED,
                                      .int_type = target});
}

void event_logger_driver_on_logger_event_trace(event_logger_driver_ref self,
                                               int domain_id, int event_id)
{
    _logger_append(self, &(_record_t){.is_trace = true, .event_id = event_id});
}

void event_logger_driver_on_bus_oamdma(event_logger_driver_ref self,
                                       uint16_t addr)
{
    _logger_append(
        self, &(_record_t){.event_id = EVENT_ID_BUS_OAMDMA, .dma_addr = addr});
}

void event_logger_driver_on_bus_dmcdma(event_logger_driver_ref self,
                                       uint16_t addr)
{
    _logger_append(
        self, &(_record_t){.event_id = EVENT_ID_BUS_DMCDMA, .dma_addr = addr});
}

void event_logger_driver_trace_enable(event_logger_driver_ref self)
{
    core_event_log_enable(self->shared_core);
    self->trace_enabled = true;
}

void event_logger_driver_trace_disable(event_logger_driver_ref self)
{
    core_event_log_disable(self->shared_core);
    self->trace_enabled = false;
}

void event_logger_driver_read_start(event_logger_driver_ref self)
{
    ring_logger_init_cursor(self->rlogger);
}

static void _show_log(event_logger_driver_ref self, const _record_t *log,
                      char *buf, size_t len)
{
    int offset = snprintf(buf, len, "%3d/%3d@%04x ", log->scanline.x,
                          log->scanline.y, log->pc);

    if ((offset < 0) || (offset >= len - 1)) {
        return;
    }

    buf = &buf[offset];
    len -= offset;
    if (log->is_trace) {
        snprintf(buf, len, "%s.%s", event_domain_id_show(log->domain_id),
                 event_id_show(log->event_id));
    } else {
        switch (log->event_id) {
        case EVENT_ID_BUS_INT_ACKED:
            snprintf(buf, len, "-%s", interruption_type_show(log->int_type));
            break;
        case EVENT_ID_BUS_INT_ASSERTED:
            snprintf(buf, len, "!%s", interruption_type_show(log->int_type));
            break;
        case EVENT_ID_BUS_OAMDMA:
            snprintf(buf, len, "OAMDMA %04x", log->dma_addr);
            break;
        case EVENT_ID_BUS_DMCDMA:
            snprintf(buf, len, "DMCDMA %04x", log->dma_addr);
            break;
        case EVENT_ID_PPU_REG:
            snprintf(buf, len, "%s%s%02x", ppu_reg_addr_show(log->ppu_reg.reg),

                     log->ppu_reg.is_read ? "->" : "<-", log->ppu_reg.value);
            break;
        case EVENT_ID_PPU_VBLANK:
            snprintf(buf, len, "VBL");
            break;
        case EVENT_ID_PPU_SPRITE0_HIT:
            snprintf(buf, len, "+SPRITE0");
            break;
        default:
            break;
        }
    }
}

bool event_logger_driver_read(event_logger_driver_ref self, char *buf,
                              size_t len)
{
    const _record_t *log = ring_logger_read(self->rlogger);

    if (!log) {
        return false;
    }

    _show_log(self, log, buf, len);

    return true;
}
