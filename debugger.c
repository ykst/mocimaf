#include "debugger.h"
#include "bus_internal.h"

typedef struct debugger_entry {
    struct {
        uint8_t breakpoint : 1;
        uint8_t breakpoint_hit : 1;
    };
} debugger_entry_t;

typedef struct debugger {
    debugger_entry_t *entries;
    bus_ref shared_bus;

    struct {
        bool enabled;
        int dot;
        int line;
    } scanline_break;
    bool scanline_before;

    struct {
        bool nmi;
        bool rst;
        bool irq;
    } interruption_break;

    debugger_step_mode_t step_mode;
} debugger_t;

void debugger_destroy(debugger_ref self)
{
    if (self) {
        FREE(self->entries);
        FREE(self);
    }
}

debugger_ref debugger_create(bus_ref bus)
{
    debugger_ref self = NULL;

    TALLOC(self);
    GUARD(self->shared_bus = bus);
    TALLOCS(self->entries, bus_prg_size(self->shared_bus));

    return self;
error:
    debugger_destroy(self);
    return NULL;
}

debugger_step_mode_t debugger_get_step_mode(debugger_ref self)
{
    return self->step_mode;
}

void debugger_set_step_mode(debugger_ref self, debugger_step_mode_t step_mode)
{
    self->step_mode = step_mode;
}
void debugger_breakpoint_unhit(debugger_ref self, uint16_t virt)
{
    uint32_t phys = bus_prg_phys(self->shared_bus, virt);
    self->entries[phys].breakpoint_hit = 0;
}

bool debugger_check_breakpoint_hit(debugger_ref self, uint16_t virt)
{
    uint32_t phys = bus_prg_phys(self->shared_bus, virt);
    debugger_entry_t *entry = &self->entries[phys];

    if (entry->breakpoint) {
        if (entry->breakpoint_hit) {
            entry->breakpoint_hit = 0;
            TRACE("got through breakpoint %04x@%05x\n", virt, phys);
        } else {
            TRACE("hit breakpoint %04x@%05x\n", virt, phys);
            entry->breakpoint_hit = 1;

            // debugger_event_emit_breakpoint(self->shared_emitter, self);

            return true;
        }
    }

    return false;
}

bool debugger_is_breakpoint(debugger_ref self, uint32_t phys)
{
    return self->entries[phys % bus_prg_size(self->shared_bus)].breakpoint;
}

bool debugger_breakpoint_toggle(debugger_ref self, uint32_t phys)
{
    debugger_entry_t *entry =
        &self->entries[phys % bus_prg_size(self->shared_bus)];
    entry->breakpoint = !entry->breakpoint;
    return entry->breakpoint;
}

bool debugger_interruption_breakpoint_toggle(debugger_ref self,
                                             interruption_type_t type)
{
    bool ret = false;

    switch (type) {
    case INTERRUTPION_TYPE_NMI:
        ret = negate(&self->interruption_break.nmi);
        break;
    case INTERRUTPION_TYPE_RST:
        ret = negate(&self->interruption_break.rst);
        break;
    case INTERRUTPION_TYPE_IRQ:
        ret = negate(&self->interruption_break.irq);
        break;
    default:
        WARN("undefined interruption type: %d\n", type);
    }

    return ret;
}

bool debugger_check_interruption_break(debugger_ref self,
                                       interruption_type_t type)
{
    bool ret = false;

    switch (type) {
    case INTERRUTPION_TYPE_NMI:
        ret = self->interruption_break.nmi;
        break;
    case INTERRUTPION_TYPE_RST:
        ret = self->interruption_break.rst;
        break;
    case INTERRUTPION_TYPE_IRQ:
        ret = self->interruption_break.irq;
        break;
    default:
        WARN("undefined interruption type: %d\n", type);
    }

    // if (ret) {
    // debugger_event_emit_interruption_break(self->shared_emitter, self);
    //}

    return ret;
}

int debugger_set_scanline_break(debugger_ref self, int line, int dot)
{
    self->scanline_break.enabled = true;
    self->scanline_break.line = line;
    self->scanline_break.dot = dot;

    return SUCCESS;
}

void debugger_reset_scanline_break(debugger_ref self)
{
    self->scanline_break.enabled = false;
    self->scanline_break.line = 0;
    self->scanline_break.dot = 0;
}

bool debugger_check_scanline_before(debugger_ref self, int x, int y)
{
    self->scanline_before = self->scanline_break.enabled &&
                            (y <= self->scanline_break.line) &&
                            (x <= self->scanline_break.dot);
    return self->scanline_before;
}

bool debugger_check_scanline_after(debugger_ref self, int x, int y)
{
    bool hit = self->scanline_before && self->scanline_break.enabled &&
               (y >= self->scanline_break.line) &&
               (x >= self->scanline_break.dot);

    // if (hit) {
    //    debugger_event_emit_scanline_break(self->shared_emitter, self);
    //}

    return hit;
}
