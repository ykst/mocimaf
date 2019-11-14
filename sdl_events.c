#include "sdl_events.h"
#include "list.h"
#include "utils.h"
#include <SDL.h>

typedef struct sdl_events {
    list_head_t keyevent_chars[256];
    list_head_t keyevent_scans[SDL_NUM_SCANCODES];
    list_head_t mouseevents[2];
} sdl_events_t;

typedef struct sdl_keyevent_entry {
    list_t list_entry;
    int event;
    void *context;
    sdl_keyevent_handler_t handler;
} sdl_keyevent_entry_t;

static void _destroy_event_entry(list_head_t *list)
{
    list_t *e;
    while ((e = list_pop(list))) {
        sdl_keyevent_entry_t *entry = list_downcast(entry, e);
        FREE(entry);
    }
}

void sdl_events_destroy(sdl_events_ref self)
{
    if (self) {
        for (int i = 0; i < 256; ++i) {
            _destroy_event_entry(&self->keyevent_chars[i]);
        }
        for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {
            _destroy_event_entry(&self->keyevent_scans[i]);
        }

        FREE(self);
    }
}

sdl_events_ref sdl_events_create(void)
{
    sdl_events_t *self = NULL;
    TALLOC(self);
    return self;
error:
    sdl_events_destroy(self);
    return NULL;
}

int sdl_keyevents_load(sdl_events_ref self, void *context,
                       sdl_keyevent_handler_t handler, int config[][2])
{
    for (int i = 0; config[i][0] != 0; ++i) {
        int key = config[i][0];
        int event = config[i][1];
        list_head_t *list = NULL;

        if (key < 256) {
            list = &self->keyevent_chars[key];
        } else {
            int scankey = key & ~SDLK_SCANCODE_MASK;
            GUARD(scankey < SDL_NUM_SCANCODES);
            list = &self->keyevent_scans[scankey];
        }

        if (list) {
            sdl_keyevent_entry_t *entry = NULL;
            TALLOC(entry);
            *entry = (sdl_keyevent_entry_t){
                .event = event, .context = context, .handler = handler};
            list_append(list, &entry->list_entry);
        }
    }

    return SUCCESS;
error:
    return NG;
}

int sdl_mouseevents_load(sdl_events_ref self, void *context,
                         sdl_keyevent_handler_t handler, int config[][2])
{
    for (int i = 0; config[i][0] != 0; ++i) {
        int key = config[i][0];
        int event = config[i][1];
        list_head_t *list = NULL;

        switch (key) {
        case SDL_BUTTON_LEFT:
            list = &self->mouseevents[0];
            break;
        case SDL_BUTTON_RIGHT:
            list = &self->mouseevents[1];
            break;
        default:
            break;
        }

        if (list) {
            sdl_keyevent_entry_t *entry = NULL;

            TALLOC(entry);
            *entry = (sdl_keyevent_entry_t){
                .event = event, .context = context, .handler = handler};
            list_append(list, &entry->list_entry);
        }
    }
    return SUCCESS;
error:
    return NG;
}

int sdl_mouseevent_emit(sdl_events_ref self, SDL_Event e)
{
    list_head_t *list = NULL;

    switch (e.button.button) {
    case SDL_BUTTON_LEFT:
        list = &self->mouseevents[0];
        break;
    case SDL_BUTTON_RIGHT:
        list = &self->mouseevents[1];
        break;
    default:
        break;
    }

    if (list != NULL) {
        sdl_keyevent_entry_t *entry;

        list_foreach(list, entry)
        {
            GUARD(entry->handler(entry->context, entry->event,
                                 e.type == SDL_MOUSEBUTTONDOWN));
        }
    }

    return SUCCESS;
error:
    return NG;
}

static int sdl_keyevent_emit(sdl_events_ref self, SDL_Event e)
{
    SDL_Keycode key = e.key.keysym.sym;
    list_head_t *list = NULL;

    if (key < 256) {
        list = &self->keyevent_chars[key];
    } else {
        int scankey = key & ~SDLK_SCANCODE_MASK;
        GUARD(scankey < SDL_NUM_SCANCODES);
        list = &self->keyevent_scans[scankey];
    }

    if (list != NULL) {
        sdl_keyevent_entry_t *entry;

        list_foreach(list, entry)
        {
            GUARD(entry->handler(entry->context, entry->event,
                                 e.type == SDL_KEYDOWN));
        }
    }

    return SUCCESS;
error:
    return NG;
}

int sdl_events_emit(sdl_events_ref self, SDL_Event e)
{
    switch (e.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        GUARD(sdl_keyevent_emit(self, e));
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        GUARD(sdl_mouseevent_emit(self, e));
        break;
    default:
        break;
    }

    return SUCCESS;
error:
    return NG;
}
