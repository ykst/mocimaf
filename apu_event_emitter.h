// *** WARNING ***
// This file is generated by genevents.rb
// Keep untouched or ruin the abstraction
#pragma once
#include "event_id.h"
#include "apu_event_listener.h"
#include "list.h"
#include "logger_event_emitter.h"

typedef struct _apu_event_listener_on_sample {
    list_t list_entry;
    void *context;
    apu_event_callback_on_sample_t callback;
} _apu_event_listener_on_sample_t;
typedef struct _apu_event_listener_on_level30 {
    list_t list_entry;
    void *context;
    apu_event_callback_on_level30_t callback;
} _apu_event_listener_on_level30_t;

typedef struct apu_event_emitter {
    list_head_t sample_listeners;
    list_head_t level30_listeners;
    logger_event_emitter_t *shared_logger_emitter;
} apu_event_emitter_t;

static inline int apu_event_attach_listener(apu_event_emitter_t *self, apu_event_listener_t *listener)
{
    if (listener->on_sample) {
        _apu_event_listener_on_sample_t *elem = NULL;
        TALLOC(elem);
        elem->context = listener->context;
        elem->callback = listener->on_sample;
        list_append(&self->sample_listeners, &elem->list_entry);
    }
    if (listener->on_level30) {
        _apu_event_listener_on_level30_t *elem = NULL;
        TALLOC(elem);
        elem->context = listener->context;
        elem->callback = listener->on_level30;
        list_append(&self->level30_listeners, &elem->list_entry);
    }
    return SUCCESS;
error:
    return NG;
}

static inline void apu_event_set_logger_emitter(apu_event_emitter_t *self, logger_event_emitter_t *logger_emitter)
{
    self->shared_logger_emitter = logger_emitter;
}

// called on generated a sample (pulse and triangle-noise-dmc)
static inline void apu_event_emit_sample(apu_event_emitter_t *self, apu_ref apu, uint8_t pulse, uint8_t tnd)
{
    _apu_event_listener_on_sample_t *elem = NULL;
    list_foreach (&self->sample_listeners, elem) {
        elem->callback(elem->context, apu, pulse, tnd);
    }
}
// emits sum of the level of every 30 samples in floating point (0~30)
static inline void apu_event_emit_level30(apu_event_emitter_t *self, apu_ref apu, float level)
{
    _apu_event_listener_on_level30_t *elem = NULL;
    list_foreach (&self->level30_listeners, elem) {
        elem->callback(elem->context, apu, level);
    }
}

static inline void apu_event_detach_listener(apu_event_emitter_t *self, void *context)
{
    {
        _apu_event_listener_on_sample_t *elem = NULL;

        list_foreach (&self->sample_listeners, elem) {
            if (elem->context == context) {
                list_remove(&self->sample_listeners, &elem->list_entry);
                FREE(elem);
            }
        }
    }
    {
        _apu_event_listener_on_level30_t *elem = NULL;

        list_foreach (&self->level30_listeners, elem) {
            if (elem->context == context) {
                list_remove(&self->level30_listeners, &elem->list_entry);
                FREE(elem);
            }
        }
    }
}

static inline void apu_event_detach_listener_on_sample(apu_event_emitter_t *self, void *context)
{
    _apu_event_listener_on_sample_t *elem = NULL;
    list_foreach (&self->sample_listeners, elem) {
        if (elem->context == context) {
            list_remove(&self->sample_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
}
static inline void apu_event_detach_listener_on_level30(apu_event_emitter_t *self, void *context)
{
    _apu_event_listener_on_level30_t *elem = NULL;
    list_foreach (&self->level30_listeners, elem) {
        if (elem->context == context) {
            list_remove(&self->level30_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
}

static inline void apu_event_detach_all(apu_event_emitter_t *self)
{
    {
        _apu_event_listener_on_sample_t *elem = NULL;

        list_foreach (&self->sample_listeners, elem) {
            list_remove(&self->sample_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
    {
        _apu_event_listener_on_level30_t *elem = NULL;

        list_foreach (&self->level30_listeners, elem) {
            list_remove(&self->level30_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
}
