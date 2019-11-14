#include "playback_driver_impl.h"
#include "playlog.h"

typedef struct playback_driver {
    playback_ref playback;
    core_t *shared_core;
} playback_driver_t;

static inline void _playback(playback_driver_ref self)
{
    if (playback_finished(self->playback)) {
        core_pause(self->shared_core);
        playback_driver_unplug(self->shared_core, self);
        return;
    }

    uint8_t system;
    joypad_reg_t joypad1;
    joypad_reg_t joypad2;

    playback_step(self->playback, &system, &joypad1, &joypad2);

    input_set_controls(self->shared_core->input, system, joypad1, joypad2);
}

void playback_driver_on_ppu_frame(playback_driver_ref self, ppu_ref ppu,
                                  uint64_t frames)
{
    _playback(self);
}

void playback_driver_on_core_start(playback_driver_ref self, core_ref core)
{
    _playback(self);
}

void playback_driver_destroy(playback_driver_ref self)
{
    if (self) {
        playback_destroy(self->playback);
        FREE(self);
    }
}

playback_driver_ref playback_driver_create(core_t *core,
                                           const char *record_path)
{
    playback_driver_ref self = NULL;

    TALLOC(self);
    GUARD(self->shared_core = core);
    GUARD(self->playback = playback_create(record_path));

    return self;
error:
    playback_driver_destroy(self);
    return NULL;
}
