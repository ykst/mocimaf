#include "analyzer_driver_impl.h"
#include "console.h"
#include "debugger.h"
#include "disas.h"
#include "event_logger_driver.h"

typedef struct analyzer_driver {
    core_ref shared_core;
    debugger_ref debugger;
    disas_ref disas;
    console_ref console;
    event_logger_driver_ref event_logger;
} analyzer_driver_t;

#define _LOG_STR_LEN (24)

void analyzer_driver_destroy(analyzer_driver_ref self)
{
    if (self) {
        debugger_destroy(self->debugger);
        disas_destroy(self->disas);
        console_destroy(self->console);
        FREE(self);
    }
}

analyzer_driver_ref analyzer_driver_create(core_ref core)
{
    analyzer_driver_ref self = NULL;

    TALLOC(self);
    GUARD(self->shared_core = core);
    GUARD(self->debugger = debugger_create(core->bus));
    GUARD(self->disas = disas_create(core->mapper));
    GUARD(self->event_logger = event_logger_driver_plug(self->shared_core));
    GUARD(self->console = console_create(core, self->debugger, self->disas,
                                         self->event_logger));

    return self;
error:
    analyzer_driver_destroy(self);
    return NULL;
}

void analyzer_driver_on_cpu_read(analyzer_driver_ref self, cpu_ref cpu,
                                 uint16_t addr, size_t length)
{
    if (addr >= 0x8000) {
        disas_mark_data_runtime(self->disas, addr, length);
    }
}

void analyzer_driver_on_cpu_write(analyzer_driver_ref self, cpu_ref cpu,
                                  uint16_t addr, size_t length)
{
}

void analyzer_driver_on_core_pause(analyzer_driver_ref self, core_ref core)
{
}

void analyzer_driver_on_core_resume(analyzer_driver_ref self, core_ref core)
{
    console_set_pause_reason(self->console, CONSOLE_PAUSE_REASON_NONE);
}

static inline void _console_update(analyzer_driver_ref self)
{
    debugger_set_step_mode(self->debugger, DEBUGGER_STEP_MODE_NONE);

    console_update(self->console);

    if (debugger_get_step_mode(self->debugger) != DEBUGGER_STEP_MODE_NONE) {
        core_resume(self->shared_core);
    }
}

void analyzer_driver_on_core_poll_in_pause(analyzer_driver_ref self,
                                           core_ref core)
{
    _console_update(self);
}

void analyzer_driver_on_core_start(analyzer_driver_ref self, core_ref core)
{
    disas_mark_from_vectors(self->disas);
    _console_update(self);
}

void analyzer_driver_on_ppu_frame(analyzer_driver_ref self, ppu_ref ppu,
                                  uint64_t frames)
{
    if (debugger_get_step_mode(self->debugger) == DEBUGGER_STEP_MODE_FRAME) {
        core_pause(self->shared_core);
    }

    _console_update(self);
}

void analyzer_driver_on_ppu_scanline(analyzer_driver_ref self, ppu_ref ppu,
                                     int line, const uint16_t *dots)
{
    if (debugger_get_step_mode(self->debugger) == DEBUGGER_STEP_MODE_SCANLINE) {
        core_pause(self->shared_core);
    }
}

void analyzer_driver_on_cpu_decode(analyzer_driver_ref self, cpu_ref cpu,
                                   uint16_t pc)
{
    disas_mark_op_runtime(self->disas, cpu->reg.pc, cpu->last_op == OP_20_JSR);

    if (debugger_check_breakpoint_hit(self->debugger, pc)) {
        core_pause(self->shared_core);
        console_set_pause_reason(self->console,
                                 CONSOLE_PAUSE_REASON_BREAKPOINT);
        console_focus_pc(self->console);
    }

    debugger_check_scanline_before(self->debugger,
                                   self->shared_core->ppu->scanline.x,
                                   self->shared_core->ppu->scanline.y);
}

void analyzer_driver_on_cpu_step(analyzer_driver_ref self, cpu_ref cpu)
{
    if (debugger_get_step_mode(self->debugger) ==
        DEBUGGER_STEP_MODE_INSTRUCTION) {
        core_pause(self->shared_core);
    }

    if (debugger_check_scanline_after(self->debugger,
                                      self->shared_core->ppu->scanline.x,
                                      self->shared_core->ppu->scanline.y)) {

        core_pause(self->shared_core);
        console_focus_pc(self->console);
        console_set_pause_reason(self->console,
                                 CONSOLE_PAUSE_REASON_SCANLINE_BREAK);
    }
}

void analyzer_driver_on_cpu_interrupt(analyzer_driver_ref self, cpu_ref cpu,
                                      interruption_type_t acks)
{
    debugger_breakpoint_unhit(self->debugger, cpu->reg.pc);

    if (debugger_check_interruption_break(self->debugger, acks)) {
        core_pause(self->shared_core);
        console_focus_pc(self->console);
        console_set_pause_reason(self->console,
                                 CONSOLE_PAUSE_REASON_INTERRUPTION_BREAK);
    }
}
