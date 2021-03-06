// *** WARNING ***
// This file is generated by gendriver.rb
// Keep untouched or ruin the abstraction
#include "drivers.h"
#include "event_logger_driver_impl.h"

event_logger_driver_ref event_logger_driver_plug(core_ref core)
{
    event_logger_driver_ref driver = NULL;

    GUARD(driver = event_logger_driver_create(core));

    bus_event_attach_listener(&core->events.bus,
        &(bus_event_listener_t) {
            .context = driver,
                    .on_int_asserted = (bus_event_callback_on_int_asserted_t)event_logger_driver_on_bus_int_asserted,
                .on_int_acked = (bus_event_callback_on_int_acked_t)event_logger_driver_on_bus_int_acked,
                .on_oamdma = (bus_event_callback_on_oamdma_t)event_logger_driver_on_bus_oamdma,
                .on_dmcdma = (bus_event_callback_on_dmcdma_t)event_logger_driver_on_bus_dmcdma,
        });
    cpu_event_attach_listener(&core->events.cpu,
        &(cpu_event_listener_t) {
            .context = driver,
                    .on_decode = (cpu_event_callback_on_decode_t)event_logger_driver_on_cpu_decode,
                .on_read = (cpu_event_callback_on_read_t)event_logger_driver_on_cpu_read,
                .on_write = (cpu_event_callback_on_write_t)event_logger_driver_on_cpu_write,
        });
    ppu_event_attach_listener(&core->events.ppu,
        &(ppu_event_listener_t) {
            .context = driver,
                    .on_sprite0_hit = (ppu_event_callback_on_sprite0_hit_t)event_logger_driver_on_ppu_sprite0_hit,
                .on_vblank = (ppu_event_callback_on_vblank_t)event_logger_driver_on_ppu_vblank,
                .on_reg = (ppu_event_callback_on_reg_t)event_logger_driver_on_ppu_reg,
        });
    logger_event_attach_listener(&core->events.logger,
        &(logger_event_listener_t) {
            .context = driver,
                    .on_event_trace = (logger_event_callback_on_event_trace_t)event_logger_driver_on_logger_event_trace,
        });

    core_event_attach_listener(&core->events.core, &(core_event_listener_t) {
        .context = driver,
        .on_destroy = (core_event_callback_on_destroy_t)event_logger_driver_destroy,
    });

    return driver;
error:
    event_logger_driver_destroy(driver);
    return NULL;
}

void event_logger_driver_detach_bus_int_asserted(event_logger_driver_ref self, core_ref core)
{
    bus_event_emitter_t *emitter = &core->events.bus;
    bus_event_detach_listener_on_int_asserted(emitter, self);
}
void event_logger_driver_detach_bus_int_acked(event_logger_driver_ref self, core_ref core)
{
    bus_event_emitter_t *emitter = &core->events.bus;
    bus_event_detach_listener_on_int_acked(emitter, self);
}
void event_logger_driver_detach_bus_oamdma(event_logger_driver_ref self, core_ref core)
{
    bus_event_emitter_t *emitter = &core->events.bus;
    bus_event_detach_listener_on_oamdma(emitter, self);
}
void event_logger_driver_detach_bus_dmcdma(event_logger_driver_ref self, core_ref core)
{
    bus_event_emitter_t *emitter = &core->events.bus;
    bus_event_detach_listener_on_dmcdma(emitter, self);
}
void event_logger_driver_detach_cpu_decode(event_logger_driver_ref self, core_ref core)
{
    cpu_event_emitter_t *emitter = &core->events.cpu;
    cpu_event_detach_listener_on_decode(emitter, self);
}
void event_logger_driver_detach_cpu_read(event_logger_driver_ref self, core_ref core)
{
    cpu_event_emitter_t *emitter = &core->events.cpu;
    cpu_event_detach_listener_on_read(emitter, self);
}
void event_logger_driver_detach_cpu_write(event_logger_driver_ref self, core_ref core)
{
    cpu_event_emitter_t *emitter = &core->events.cpu;
    cpu_event_detach_listener_on_write(emitter, self);
}
void event_logger_driver_detach_ppu_sprite0_hit(event_logger_driver_ref self, core_ref core)
{
    ppu_event_emitter_t *emitter = &core->events.ppu;
    ppu_event_detach_listener_on_sprite0_hit(emitter, self);
}
void event_logger_driver_detach_ppu_vblank(event_logger_driver_ref self, core_ref core)
{
    ppu_event_emitter_t *emitter = &core->events.ppu;
    ppu_event_detach_listener_on_vblank(emitter, self);
}
void event_logger_driver_detach_ppu_reg(event_logger_driver_ref self, core_ref core)
{
    ppu_event_emitter_t *emitter = &core->events.ppu;
    ppu_event_detach_listener_on_reg(emitter, self);
}
void event_logger_driver_detach_logger_event_trace(event_logger_driver_ref self, core_ref core)
{
    logger_event_emitter_t *emitter = &core->events.logger;
    logger_event_detach_listener_on_event_trace(emitter, self);
}

void event_logger_driver_unplug(core_ref core, event_logger_driver_ref driver)
{
    event_logger_driver_detach_bus_int_asserted(driver, core);
    event_logger_driver_detach_bus_int_acked(driver, core);
    event_logger_driver_detach_bus_oamdma(driver, core);
    event_logger_driver_detach_bus_dmcdma(driver, core);
    event_logger_driver_detach_cpu_decode(driver, core);
    event_logger_driver_detach_cpu_read(driver, core);
    event_logger_driver_detach_cpu_write(driver, core);
    event_logger_driver_detach_ppu_sprite0_hit(driver, core);
    event_logger_driver_detach_ppu_vblank(driver, core);
    event_logger_driver_detach_ppu_reg(driver, core);
    event_logger_driver_detach_logger_event_trace(driver, core);
    core_event_detach_listener_on_destroy(&core->events.core, driver);

    event_logger_driver_destroy(driver);
}
