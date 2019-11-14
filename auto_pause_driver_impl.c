#include "auto_pause_driver_impl.h"

typedef struct auto_pause_driver {
    core_t *shared_core;
    uint64_t target_frames;
    uint64_t target_cpu_cycles;
    bool quit_on_pause;
    bool pause_on_start;
} auto_pause_driver_t;

void auto_pause_driver_on_ppu_frame(auto_pause_driver_ref self, ppu_ref ppu,
                                    uint64_t frames)
{
    if (self->target_frames > 0 && self->target_frames <= frames) {
        core_pause(self->shared_core);
        auto_pause_driver_detach_ppu_frame(self, self->shared_core);
    }
}

void auto_pause_driver_on_cpu_step(auto_pause_driver_ref self, cpu_ref cpu)
{
    if (self->target_cpu_cycles > 0 &&
        self->target_cpu_cycles <= cpu->shared_counter->cycles) {
        core_pause(self->shared_core);
        auto_pause_driver_detach_cpu_step(self, self->shared_core);
    }
}

void auto_pause_driver_destroy(auto_pause_driver_ref self)
{
    if (self) {
        FREE(self);
    }
}

void auto_pause_driver_on_core_pause(auto_pause_driver_ref self, core_ref core)
{
    if (self->quit_on_pause) {
        core_quit(core);
    }
}

void auto_pause_driver_on_core_start(auto_pause_driver_ref self, core_ref core)
{
    if (self->pause_on_start) {
        core_pause(core);
    }
    auto_pause_driver_detach_core_start(self, self->shared_core);
}

auto_pause_driver_ref auto_pause_driver_create(core_t *core, const args_t *args)
{
    auto_pause_driver_ref self = NULL;

    TALLOC(self);

    GUARD(self->shared_core = core);

    self->target_frames = args->skip_frames;

    self->target_cpu_cycles = args->skip_cycles;

    self->quit_on_pause = args->quiet;
    self->pause_on_start = args->pause_start;

    return self;
error:
    auto_pause_driver_destroy(self);
    return NULL;
}
