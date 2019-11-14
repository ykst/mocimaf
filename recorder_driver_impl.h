// *** WARNING ***
// This file is generated by gendriver.rb
// Keep untouched or ruin the abstraction
#pragma once
#include "utils.h"
#include "drivers.h"
#include "core.h"

// generated event detacher
void recorder_driver_detach_ppu_frame(recorder_driver_ref self, core_ref core);
void recorder_driver_detach_core_start(recorder_driver_ref self, core_ref core);

// required implementations
void recorder_driver_destroy(recorder_driver_ref self);
recorder_driver_ref recorder_driver_create(core_ref core, const char * record_out);
void recorder_driver_on_ppu_frame(recorder_driver_ref self, ppu_ref ppu, uint64_t frames);
void recorder_driver_on_core_start(recorder_driver_ref self, core_ref core);