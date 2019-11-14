#include "core.h"

#include "apu.h"
#include "cpu.h"
#include "ppu.h"
// bus
#include "bus_internal.h"
#include "interrupts.h"
#include "mapper.h"

void core_destroy(core_ref self)
{
    if (self) {
        core_event_emit_destroy(&self->events.core);
        events_detach_all(&self->events);
        apu_destroy(self->apu);
        cpu_destroy(self->cpu);
        ppu_destroy(self->ppu);
        input_destroy(self->input);
        // debugger_destroy(self->debugger);
        bus_destroy(self->bus);

        FREE(self);
    }
}

core_ref core_create(const ines_t *ines, const args_t *args)
{
    core_ref self = NULL;

    TALLOC(self);

    GUARD(self->mapper = mapper_create(ines));
    GUARD(self->shared_ines = ines);

    GUARD(self->ppu = ppu_create(&self->events.ppu));
    GUARD(self->apu = apu_create(&self->events.apu));
    GUARD(self->cpu = cpu_create(&self->events.cpu));
    GUARD(self->input = input_create(&self->events.input));

    GUARD(self->bus = bus_create(&self->events.bus, self->mapper, self->cpu,
                                 self->apu, self->ppu, self->input));

    return self;
error:
    core_destroy(self);
    return NULL;
}

int core_cold_boot(core_t *self)
{
    GUARD(cpu_cold_boot(self->cpu));
    GUARD(ppu_cold_boot(self->ppu));
    GUARD(apu_cold_boot(self->apu));

    return SUCCESS;
error:
    return NG;
}

int core_run(core_ref self)
{
    cpu_t *cpu = self->cpu;

    GUARD(core_cold_boot(self));

    core_event_emit_start(&self->events.core, self);

    while (!self->quit) {
        if (self->paused) {
            usleep(16666);
            core_event_emit_poll_in_pause(&self->events.core, self);
            if (!self->paused) {
                core_event_emit_resume(&self->events.core, self);
            }
        } else {
            self->bus->paused = false;
            while (!self->paused) {
                cpu_clock(cpu);
            }

            core_event_emit_pause(&self->events.core, self);
        }
    }

    core_event_emit_quit(&self->events.core, self);

    return SUCCESS;
error:
    return NG;
}

void core_reset(core_ref self)
{
    bus_assert_rst(self->bus);
}

bool core_toggle_mute_pulse1(core_ref self)
{
    return negate(&self->apu->channel_mute.pulse1);
}

bool core_toggle_mute_pulse2(core_ref self)
{
    return negate(&self->apu->channel_mute.pulse2);
}

bool core_toggle_mute_triangle(core_ref self)
{
    return negate(&self->apu->channel_mute.triangle);
}

bool core_toggle_mute_dmc(core_ref self)
{
    return negate(&self->apu->channel_mute.dmc);
}

bool core_toggle_mute_noise(core_ref self)
{
    return negate(&self->apu->channel_mute.noise);
}

bool core_toggle_hide_sprites(core_ref self)
{
    return negate(&self->ppu->hide_sprites);
}

bool core_toggle_hide_background(core_ref self)
{
    return negate(&self->ppu->hide_bg);
}

void core_event_log_enable(core_ref self)
{
    ppu_event_set_logger_emitter(&self->events.ppu, &self->events.logger);
    cpu_event_set_logger_emitter(&self->events.cpu, &self->events.logger);
    apu_event_set_logger_emitter(&self->events.apu, &self->events.logger);
    input_event_set_logger_emitter(&self->events.input, &self->events.logger);
    core_event_set_logger_emitter(&self->events.core, &self->events.logger);
}

void core_event_log_disable(core_ref self)
{
    ppu_event_set_logger_emitter(&self->events.ppu, NULL);
    cpu_event_set_logger_emitter(&self->events.cpu, NULL);
    apu_event_set_logger_emitter(&self->events.apu, NULL);
    input_event_set_logger_emitter(&self->events.input, NULL);
    core_event_set_logger_emitter(&self->events.core, NULL);
}
