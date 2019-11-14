// *** WARNING ***
// This file is generated by gendriver.rb
// Keep untouched or ruin the abstraction
#include "drivers.h"
#include "recorder_driver_impl.h"

recorder_driver_ref recorder_driver_plug(core_ref core, const char * record_out)
{
    recorder_driver_ref driver = NULL;

    GUARD(driver = recorder_driver_create(core, record_out));

    ppu_event_attach_listener(&core->events.ppu,
        &(ppu_event_listener_t) {
            .context = driver,
                    .on_frame = (ppu_event_callback_on_frame_t)recorder_driver_on_ppu_frame,
        });
    core_event_attach_listener(&core->events.core,
        &(core_event_listener_t) {
            .context = driver,
                    .on_start = (core_event_callback_on_start_t)recorder_driver_on_core_start,
        });

    core_event_attach_listener(&core->events.core, &(core_event_listener_t) {
        .context = driver,
        .on_destroy = (core_event_callback_on_destroy_t)recorder_driver_destroy,
    });

    return driver;
error:
    recorder_driver_destroy(driver);
    return NULL;
}

void recorder_driver_detach_ppu_frame(recorder_driver_ref self, core_ref core)
{
    ppu_event_emitter_t *emitter = &core->events.ppu;
    ppu_event_detach_listener_on_frame(emitter, self);
}
void recorder_driver_detach_core_start(recorder_driver_ref self, core_ref core)
{
    core_event_emitter_t *emitter = &core->events.core;
    core_event_detach_listener_on_start(emitter, self);
}

void recorder_driver_unplug(core_ref core, recorder_driver_ref driver)
{
    recorder_driver_detach_ppu_frame(driver, core);
    recorder_driver_detach_core_start(driver, core);
    core_event_detach_listener_on_destroy(&core->events.core, driver);

    recorder_driver_destroy(driver);
}
