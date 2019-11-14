// *** WARNING ***
// This file is generated by gendriver.rb
// Keep untouched or ruin the abstraction
#include "drivers.h"
#include "disas_driver_impl.h"

disas_driver_ref disas_driver_plug(core_ref core, const char * out_path)
{
    disas_driver_ref driver = NULL;

    GUARD(driver = disas_driver_create(core, out_path));

    cpu_event_attach_listener(&core->events.cpu,
        &(cpu_event_listener_t) {
            .context = driver,
                    .on_decode = (cpu_event_callback_on_decode_t)disas_driver_on_cpu_decode,
                .on_read = (cpu_event_callback_on_read_t)disas_driver_on_cpu_read,
        });
    core_event_attach_listener(&core->events.core,
        &(core_event_listener_t) {
            .context = driver,
                    .on_start = (core_event_callback_on_start_t)disas_driver_on_core_start,
                .on_quit = (core_event_callback_on_quit_t)disas_driver_on_core_quit,
        });

    core_event_attach_listener(&core->events.core, &(core_event_listener_t) {
        .context = driver,
        .on_destroy = (core_event_callback_on_destroy_t)disas_driver_destroy,
    });

    return driver;
error:
    disas_driver_destroy(driver);
    return NULL;
}

void disas_driver_detach_cpu_decode(disas_driver_ref self, core_ref core)
{
    cpu_event_emitter_t *emitter = &core->events.cpu;
    cpu_event_detach_listener_on_decode(emitter, self);
}
void disas_driver_detach_cpu_read(disas_driver_ref self, core_ref core)
{
    cpu_event_emitter_t *emitter = &core->events.cpu;
    cpu_event_detach_listener_on_read(emitter, self);
}
void disas_driver_detach_core_start(disas_driver_ref self, core_ref core)
{
    core_event_emitter_t *emitter = &core->events.core;
    core_event_detach_listener_on_start(emitter, self);
}
void disas_driver_detach_core_quit(disas_driver_ref self, core_ref core)
{
    core_event_emitter_t *emitter = &core->events.core;
    core_event_detach_listener_on_quit(emitter, self);
}

void disas_driver_unplug(core_ref core, disas_driver_ref driver)
{
    disas_driver_detach_cpu_decode(driver, core);
    disas_driver_detach_cpu_read(driver, core);
    disas_driver_detach_core_start(driver, core);
    disas_driver_detach_core_quit(driver, core);
    core_event_detach_listener_on_destroy(&core->events.core, driver);

    disas_driver_destroy(driver);
}