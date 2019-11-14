// *** WARNING ***
// This file is generated by gendriver.rb
// Keep untouched or ruin the abstraction
#pragma once
#include "utils.h"
#include "core.h"
#include "args.h"

// 
typedef struct tracer_driver *tracer_driver_ref;
tracer_driver_ref tracer_driver_plug(core_ref core, const args_t * args);
void tracer_driver_unplug(core_ref core, tracer_driver_ref self);
// 
typedef struct auto_pause_driver *auto_pause_driver_ref;
auto_pause_driver_ref auto_pause_driver_plug(core_ref core, const args_t * args);
void auto_pause_driver_unplug(core_ref core, auto_pause_driver_ref self);
// 
typedef struct playback_driver *playback_driver_ref;
playback_driver_ref playback_driver_plug(core_ref core, const char * record_in);
void playback_driver_unplug(core_ref core, playback_driver_ref self);
// 
typedef struct recorder_driver *recorder_driver_ref;
recorder_driver_ref recorder_driver_plug(core_ref core, const char * record_out);
void recorder_driver_unplug(core_ref core, recorder_driver_ref self);
// 
typedef struct sdl_driver *sdl_driver_ref;
sdl_driver_ref sdl_driver_plug(core_ref core, args_t * args);
void sdl_driver_unplug(core_ref core, sdl_driver_ref self);
// 
typedef struct analyzer_driver *analyzer_driver_ref;
analyzer_driver_ref analyzer_driver_plug(core_ref core);
void analyzer_driver_unplug(core_ref core, analyzer_driver_ref self);
// 
typedef struct disas_driver *disas_driver_ref;
disas_driver_ref disas_driver_plug(core_ref core, const char * out_path);
void disas_driver_unplug(core_ref core, disas_driver_ref self);
// 
typedef struct event_logger_driver *event_logger_driver_ref;
event_logger_driver_ref event_logger_driver_plug(core_ref core);
void event_logger_driver_unplug(core_ref core, event_logger_driver_ref self);