// *** WARNING ***
// This file is generated by gendriver.rb
// Keep untouched or ruin the abstraction
#include "drivers.h"
#include "sdl_driver_impl.h"

sdl_driver_ref sdl_driver_plug(core_ref core, args_t * args)
{
    sdl_driver_ref driver = NULL;

    GUARD(driver = sdl_driver_create(core, args));

    ppu_event_attach_listener(&core->events.ppu,
        &(ppu_event_listener_t) {
            .context = driver,
                    .on_frame = (ppu_event_callback_on_frame_t)sdl_driver_on_ppu_frame,
                .on_scanline = (ppu_event_callback_on_scanline_t)sdl_driver_on_ppu_scanline,
        });
    core_event_attach_listener(&core->events.core,
        &(core_event_listener_t) {
            .context = driver,
                    .on_start = (core_event_callback_on_start_t)sdl_driver_on_core_start,
                .on_poll_in_pause = (core_event_callback_on_poll_in_pause_t)sdl_driver_on_core_poll_in_pause,
                .on_pause = (core_event_callback_on_pause_t)sdl_driver_on_core_pause,
                .on_resume = (core_event_callback_on_resume_t)sdl_driver_on_core_resume,
        });
    apu_event_attach_listener(&core->events.apu,
        &(apu_event_listener_t) {
            .context = driver,
                    .on_level30 = (apu_event_callback_on_level30_t)sdl_driver_on_apu_level30,
        });

    core_event_attach_listener(&core->events.core, &(core_event_listener_t) {
        .context = driver,
        .on_destroy = (core_event_callback_on_destroy_t)sdl_driver_destroy,
    });

    return driver;
error:
    sdl_driver_destroy(driver);
    return NULL;
}

void sdl_driver_detach_ppu_frame(sdl_driver_ref self, core_ref core)
{
    ppu_event_emitter_t *emitter = &core->events.ppu;
    ppu_event_detach_listener_on_frame(emitter, self);
}
void sdl_driver_detach_ppu_scanline(sdl_driver_ref self, core_ref core)
{
    ppu_event_emitter_t *emitter = &core->events.ppu;
    ppu_event_detach_listener_on_scanline(emitter, self);
}
void sdl_driver_detach_core_start(sdl_driver_ref self, core_ref core)
{
    core_event_emitter_t *emitter = &core->events.core;
    core_event_detach_listener_on_start(emitter, self);
}
void sdl_driver_detach_core_poll_in_pause(sdl_driver_ref self, core_ref core)
{
    core_event_emitter_t *emitter = &core->events.core;
    core_event_detach_listener_on_poll_in_pause(emitter, self);
}
void sdl_driver_detach_core_pause(sdl_driver_ref self, core_ref core)
{
    core_event_emitter_t *emitter = &core->events.core;
    core_event_detach_listener_on_pause(emitter, self);
}
void sdl_driver_detach_core_resume(sdl_driver_ref self, core_ref core)
{
    core_event_emitter_t *emitter = &core->events.core;
    core_event_detach_listener_on_resume(emitter, self);
}
void sdl_driver_detach_apu_level30(sdl_driver_ref self, core_ref core)
{
    apu_event_emitter_t *emitter = &core->events.apu;
    apu_event_detach_listener_on_level30(emitter, self);
}

void sdl_driver_unplug(core_ref core, sdl_driver_ref driver)
{
    sdl_driver_detach_ppu_frame(driver, core);
    sdl_driver_detach_ppu_scanline(driver, core);
    sdl_driver_detach_core_start(driver, core);
    sdl_driver_detach_core_poll_in_pause(driver, core);
    sdl_driver_detach_core_pause(driver, core);
    sdl_driver_detach_core_resume(driver, core);
    sdl_driver_detach_apu_level30(driver, core);
    core_event_detach_listener_on_destroy(&core->events.core, driver);

    sdl_driver_destroy(driver);
}