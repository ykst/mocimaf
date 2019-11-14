#include "tracer_driver_impl.h"
#include "tracer.h"
#include <string.h>

typedef struct tracer_driver {
    core_ref shared_core;
    tracer_ref tracer;
    struct {
        bool cpu;
        bool apu;
        bool scanline;
        uint64_t target_frame;
    } verbose_trace;
} tracer_driver_t;

void tracer_driver_destroy(tracer_driver_ref self)
{
    if (self) {
        tracer_destroy(self->tracer);
        FREE(self);
    }
}

tracer_driver_ref tracer_driver_create(core_ref core, const args_t *args)
{
    tracer_driver_ref self = NULL;

    TALLOC(self);
    GUARD(self->shared_core = core);
    GUARD(self->tracer = tracer_create());
    self->verbose_trace.cpu = args->verbose_trace_mode == 1;
    self->verbose_trace.apu = args->verbose_trace_mode == 2;
    self->verbose_trace.scanline = args->verbose_trace_mode == 3;
    self->verbose_trace.target_frame = args->skip_frames - 1;

    return self;
error:
    tracer_driver_destroy(self);
    return NULL;
}

void tracer_driver_on_core_start(tracer_driver_ref self, core_ref core)
{
    tracer_start(self->tracer);
    tracer_driver_detach_core_start(self, core);
}

void tracer_driver_on_cpu_step(tracer_driver_ref self, cpu_ref cpu)
{
    tracer_cpu_trace(self->tracer, cpu);

    if (self->verbose_trace.cpu &&
        (self->verbose_trace.target_frame == self->shared_core->ppu->frames)) {
        tracer_verbose_flush_cpu(self->tracer, cpu);
    }
}

void tracer_driver_on_apu_sample(tracer_driver_ref self, apu_ref apu,
                                 uint8_t pulse, uint8_t tnd)
{
    tracer_apu_trace(self->tracer, pulse, tnd);
    if (self->verbose_trace.apu &&
        (self->verbose_trace.target_frame == self->shared_core->ppu->frames)) {
        tracer_verbose_flush_apu(self->tracer, apu);
    }
}

void tracer_driver_on_ppu_scanline(tracer_driver_ref self, ppu_ref ppu,
                                   int line, const uint16_t *dots)
{
    tracer_scanline_trace(self->tracer, line, dots);
    if (self->verbose_trace.scanline &&
        (self->verbose_trace.target_frame == self->shared_core->ppu->frames)) {
        tracer_verbose_flush_scanline(self->tracer, ppu);
    }
}

void tracer_driver_on_core_pause(tracer_driver_ref self, core_ref core)
{
    tracer_flush(self->tracer, core);
}

void tracer_driver_on_core_quit(tracer_driver_ref self, core_ref core)
{
    tracer_flush(self->tracer, core);
}
