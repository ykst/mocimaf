// *** WARNING ***
// This file is generated by gendriver.rb
// Keep untouched or ruin the abstraction
#pragma once
#include "utils.h"
#include "drivers.h"
#include "core.h"

// generated event detacher
void playback_driver_detach_ppu_frame(playback_driver_ref self, core_ref core);
void playback_driver_detach_core_start(playback_driver_ref self, core_ref core);

// required implementations
void playback_driver_destroy(playback_driver_ref self);
playback_driver_ref playback_driver_create(core_ref core, const char * record_in);
void playback_driver_on_ppu_frame(playback_driver_ref self, ppu_ref ppu, uint64_t frames);
void playback_driver_on_core_start(playback_driver_ref self, core_ref core);
