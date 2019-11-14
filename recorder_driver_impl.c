#include "recorder_driver_impl.h"
#include "playlog.h"

typedef struct recorder_driver {
    // console_ref console;
    // display_ref display;
    recorder_ref recorder;
    core_t *shared_core;
} recorder_driver_t;

static inline void _record(recorder_driver_ref self)
{
    uint8_t system = 0;

    if (self->shared_core->bus->ints.rst) {
        system |= PLAYLOG_SYSTEM_RESET;
    }

    recorder_append(self->recorder, system,
                    self->shared_core->bus->joypads[0].reg,
                    self->shared_core->bus->joypads[1].reg);
}

void recorder_driver_on_ppu_frame(recorder_driver_ref self, ppu_ref ppu,
                                  uint64_t frames)
{
    _record(self);
}

void recorder_driver_on_core_start(recorder_driver_ref self, core_ref core)
{
    _record(self);
}

void recorder_driver_destroy(recorder_driver_ref self)
{
    if (self) {
        recorder_destroy(self->recorder);
        FREE(self);
    }
}

recorder_driver_ref recorder_driver_create(core_t *core, const char *record_out)
{
    recorder_driver_ref self = NULL;

    TALLOC(self);

    GUARD(self->shared_core = core);
    GUARD(self->recorder = recorder_create(record_out));

    return self;
error:
    recorder_driver_destroy(self);
    return NULL;
}
