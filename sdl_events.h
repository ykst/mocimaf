#pragma once
#include "list.h"
#include "utils.h"
#include <SDL.h>

typedef int (*sdl_keyevent_handler_t)(void *, int, bool);
typedef struct sdl_events *sdl_events_ref;

sdl_events_ref sdl_events_create(void);
void sdl_events_destroy(sdl_events_ref self);
int sdl_keyevents_load(sdl_events_ref self, void *context,
                       sdl_keyevent_handler_t handler, int config[][2]);
int sdl_mouseevents_load(sdl_events_ref self, void *context,
                         sdl_keyevent_handler_t handler, int config[][2]);
int sdl_events_emit(sdl_events_ref self, SDL_Event e);
