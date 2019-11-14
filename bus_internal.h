#pragma once
#include "bus.h"
#include "bus_event_emitter.h"
#include "counter.h"
#include "input.h"
#include "interrupts.h"
#include "joypad.h"
#include "mapper.h"
#include "utils.h"

#include "apu.h"
#include "cpu.h"
#include "ppu.h"

typedef struct bus {
    apu_t *shared_apu;
    ppu_t *shared_ppu;
    cpu_t *shared_cpu;
    mapper_t *shared_mapper;
    input_ref shared_input;
    bus_event_emitter_t *shared_emitter;
    counters_t counters;

    uint32_t prg_size;
    uint8_t vram[0x800];
    uint8_t prg_ram[0x2000];

    uint8_t open_bus;

    joypad_t joypads[2];

    bool oamdma;
    bool paused;

    interrupts_t ints;

} bus_t;

void bus_destroy(bus_ref self);
bus_ref bus_create(bus_event_emitter_t *emitter, mapper_t *mapper, cpu_t *cpu,
                   apu_t *apu, ppu_t *ppu, input_ref input);

static inline void bus_clock_sync(bus_ref self)
{
    int delta = self->counters.cpu.cycle_offset;

    self->counters.apu.cycle_offset += delta;
    self->counters.ppu.cycle_offset += delta * 3;
    self->counters.cpu.cycles += delta;

    self->counters.apu.cycles = self->counters.cpu.cycles;
    self->counters.ppu.cycles = self->counters.cpu.cycles * 3;

    self->counters.cpu.cycle_offset = 0;
}

static inline void bus_clock_cpu(bus_ref self, int delta_cycles)
{
    self->counters.cpu.cycle_offset += delta_cycles;
}

static inline int bus_clock_coprocessors(bus_ref self)
{
    bus_clock_sync(self);
    GUARD(apu_clock(self->shared_apu));
    GUARD(ppu_clock(self->shared_ppu));
    self->oamdma = false;

    return SUCCESS;
error:
    return NG;
}

static inline void bus_oamdma(bus_ref self, uint16_t addr, uint8_t *dst)
{
    // if (self->shared_debugger) {
    //     debugger_log_append(self->shared_debugger,
    //                         (debugger_log_t){.type = EVENT_LOG_DMA_OAM,
    //                                          .payload_dma = {.virt = addr}});
    // }
    bus_event_emit_oamdma(self->shared_emitter, addr);

    for (int i = 0; i < 256; ++i) {
        int pos = (i + self->shared_ppu->reg.oamaddr) & 0xFF;
        dst[pos] = bus_read(self, addr + i);
    }

    bus_clock_cpu(self, 257 + (self->counters.cpu.cycles & 1));

    self->oamdma = true;
}

static inline uint8_t bus_dmcdma(bus_ref self, uint16_t addr)
{
    bus_event_emit_dmcdma(self->shared_emitter, addr);

    uint8_t ret = bus_read(self, addr);

    // accounting 1 clock consumed by bus_read
    bus_clock_cpu(self, self->oamdma ? 1 : 3);
    bus_clock_sync(self);

    return ret;
}

static inline uint8_t *bus_fetch_chr(bus_ref self, uint16_t addr)
{
    return self->shared_mapper->on_chr_read(self->shared_mapper, addr);
}

static inline uint8_t *bus_fetch_vram_page(bus_ref self, bool nametable_x,
                                           bool nametable_y)
{
    uint16_t page_offset = 0;

    switch (self->shared_mapper->mirror_mode) {
    case VRAM_MIRROR_VERTICAL:
        page_offset = nametable_x ? 0x400 : 0;
        break;
    case VRAM_MIRROR_HORIZONTAL:
        page_offset = nametable_y ? 0x400 : 0;
        break;
    case VRAM_MIRROR_ONE_LOWER:
        page_offset = 0;
        break;
    case VRAM_MIRROR_ONE_UPPER:
        page_offset = 0x400;
        break;
    default:
        WARN("unexpected mirror mode %d\n", self->shared_mapper->mirror_mode);
        break;
    }

    return &self->vram[page_offset];
}

static inline uint8_t *bus_fetch_vram(bus_ref self, uint16_t addr)
{
    uint8_t *page = bus_fetch_vram_page(self, addr & 0x400, addr & 0x800);
    return &page[addr & 0x3FF];
}

static inline uint32_t bus_prg_phys(bus_ref self, uint16_t virt)
{
    return self->shared_mapper->on_prg_phys(self->shared_mapper, virt);
}

static inline uint32_t bus_prg_size(bus_ref self)
{
    return self->prg_size;
}

static inline void bus_soft_reset(bus_ref self)
{
    cpu_soft_reset(self->shared_cpu);
    apu_soft_reset(self->shared_apu);
    ppu_soft_reset(self->shared_ppu);
}

static inline void bus_assert_nmi(bus_t *self)
{
    self->ints.nmi = 1;
    bus_event_emit_int_asserted(self->shared_emitter, INTERRUTPION_TYPE_NMI);
}

static inline void bus_assert_rst(bus_t *self)
{
    self->ints.rst = 1;
    bus_event_emit_int_asserted(self->shared_emitter, INTERRUTPION_TYPE_RST);
}

static inline void bus_assert_ext_irq(bus_t *self)
{
    self->ints.ext = 1;
    bus_event_emit_int_asserted(self->shared_emitter,
                                INTERRUTPION_TYPE_IRQ_EXT);
}

static inline void bus_clear_ext_irq(bus_t *self)
{
    self->ints.ext = 0;
    bus_event_emit_int_acked(self->shared_emitter, INTERRUTPION_TYPE_IRQ_EXT);
}

static inline void bus_assert_frame_irq(bus_t *self)
{
    self->ints.frame = 1;
    bus_event_emit_int_asserted(self->shared_emitter,
                                INTERRUTPION_TYPE_IRQ_FRAME);
}

static inline void bus_clear_frame_irq(bus_t *self)
{
    self->ints.frame = 0;
    bus_event_emit_int_acked(self->shared_emitter, INTERRUTPION_TYPE_IRQ_FRAME);
}

static inline void bus_assert_dmc_irq(bus_t *self)
{
    self->ints.dmc = 1;
    bus_event_emit_int_asserted(self->shared_emitter,
                                INTERRUTPION_TYPE_IRQ_DMC);
}

static inline void bus_clear_dmc_irq(bus_t *self)
{
    self->ints.dmc = 0;
    bus_event_emit_int_acked(self->shared_emitter, INTERRUTPION_TYPE_IRQ_DMC);
}

static inline void bus_acknowledged_irq(bus_t *self)
{
    bus_event_emit_int_acked(self->shared_emitter, INTERRUTPION_TYPE_IRQ);
}

static inline void bus_acknowledged_rst(bus_t *self)
{
    self->ints.rst = 0;
    bus_event_emit_int_acked(self->shared_emitter, INTERRUTPION_TYPE_RST);
}

static inline void bus_acknowledged_nmi(bus_t *self)
{
    self->ints.nmi = 0;
    bus_event_emit_int_acked(self->shared_emitter, INTERRUTPION_TYPE_NMI);
}
