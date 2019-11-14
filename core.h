#pragma once

#include "args.h"
#include "ines.h"
#include "utils.h"

#include "apu.h"
#include "cpu.h"
#include "input.h"
#include "ppu.h"

// bus
#include "bus_internal.h"
#include "interrupts.h"
#include "mapper.h"

typedef struct core *core_ref;
#include "events.h"

typedef struct core {
    //    interrupts_t ints;
    mapper_t *mapper;
    ppu_t *ppu;
    apu_t *apu;
    cpu_t *cpu;
    bus_t *bus;
    input_ref input;
    events_t events;
    const ines_t *shared_ines;

    bool paused;
    bool quit;
} core_t;

void core_destroy(core_ref self);

core_ref core_create(const ines_t *ines, const args_t *args);

int core_run(core_ref self);

static inline void core_pause(core_ref self)
{
    self->paused = true;
    self->bus->paused = true;
}

static inline bool core_toggle_pause(core_ref self)
{
    return negate(&self->paused);
}

static inline void core_resume(core_ref self)
{
    self->paused = false;
    self->bus->paused = false;
}

static inline void core_quit(core_ref self)
{
    self->paused = true;
    self->quit = true;
}

void core_reset(core_ref self);

bool core_toggle_mute_pulse1(core_ref self);
bool core_toggle_mute_pulse2(core_ref self);
bool core_toggle_mute_triangle(core_ref self);
bool core_toggle_mute_dmc(core_ref self);
bool core_toggle_mute_noise(core_ref self);

bool core_toggle_hide_sprites(core_ref self);
bool core_toggle_hide_background(core_ref self);

void core_event_log_enable(core_ref self);
void core_event_log_disable(core_ref self);
