// *** WARNING ***
// This file is generated by genevents.rb
// Keep untouched or ruin the abstraction
#pragma once
#include "event_id.h"
#include "bus_event_listener.h"
#include "list.h"
#include "logger_event_emitter.h"

typedef struct _bus_event_listener_on_int_asserted {
    list_t list_entry;
    void *context;
    bus_event_callback_on_int_asserted_t callback;
} _bus_event_listener_on_int_asserted_t;
typedef struct _bus_event_listener_on_int_acked {
    list_t list_entry;
    void *context;
    bus_event_callback_on_int_acked_t callback;
} _bus_event_listener_on_int_acked_t;
typedef struct _bus_event_listener_on_oamdma {
    list_t list_entry;
    void *context;
    bus_event_callback_on_oamdma_t callback;
} _bus_event_listener_on_oamdma_t;
typedef struct _bus_event_listener_on_dmcdma {
    list_t list_entry;
    void *context;
    bus_event_callback_on_dmcdma_t callback;
} _bus_event_listener_on_dmcdma_t;

typedef struct bus_event_emitter {
    list_head_t int_asserted_listeners;
    list_head_t int_acked_listeners;
    list_head_t oamdma_listeners;
    list_head_t dmcdma_listeners;
    logger_event_emitter_t *shared_logger_emitter;
} bus_event_emitter_t;

static inline int bus_event_attach_listener(bus_event_emitter_t *self, bus_event_listener_t *listener)
{
    if (listener->on_int_asserted) {
        _bus_event_listener_on_int_asserted_t *elem = NULL;
        TALLOC(elem);
        elem->context = listener->context;
        elem->callback = listener->on_int_asserted;
        list_append(&self->int_asserted_listeners, &elem->list_entry);
    }
    if (listener->on_int_acked) {
        _bus_event_listener_on_int_acked_t *elem = NULL;
        TALLOC(elem);
        elem->context = listener->context;
        elem->callback = listener->on_int_acked;
        list_append(&self->int_acked_listeners, &elem->list_entry);
    }
    if (listener->on_oamdma) {
        _bus_event_listener_on_oamdma_t *elem = NULL;
        TALLOC(elem);
        elem->context = listener->context;
        elem->callback = listener->on_oamdma;
        list_append(&self->oamdma_listeners, &elem->list_entry);
    }
    if (listener->on_dmcdma) {
        _bus_event_listener_on_dmcdma_t *elem = NULL;
        TALLOC(elem);
        elem->context = listener->context;
        elem->callback = listener->on_dmcdma;
        list_append(&self->dmcdma_listeners, &elem->list_entry);
    }
    return SUCCESS;
error:
    return NG;
}

static inline void bus_event_set_logger_emitter(bus_event_emitter_t *self, logger_event_emitter_t *logger_emitter)
{
    self->shared_logger_emitter = logger_emitter;
}

// called on interrupt line is asseted
static inline void bus_event_emit_int_asserted(bus_event_emitter_t *self, interruption_type_t target)
{
    _bus_event_listener_on_int_asserted_t *elem = NULL;
    list_foreach (&self->int_asserted_listeners, elem) {
        elem->callback(elem->context, target);
    }
}
// called on interrupt line is acknowledged
static inline void bus_event_emit_int_acked(bus_event_emitter_t *self, interruption_type_t target)
{
    _bus_event_listener_on_int_acked_t *elem = NULL;
    list_foreach (&self->int_acked_listeners, elem) {
        elem->callback(elem->context, target);
    }
}
// called on OAMDMA is started
static inline void bus_event_emit_oamdma(bus_event_emitter_t *self, uint16_t addr)
{
    _bus_event_listener_on_oamdma_t *elem = NULL;
    list_foreach (&self->oamdma_listeners, elem) {
        elem->callback(elem->context, addr);
    }
}
// called on DMCDMA is started
static inline void bus_event_emit_dmcdma(bus_event_emitter_t *self, uint16_t addr)
{
    _bus_event_listener_on_dmcdma_t *elem = NULL;
    list_foreach (&self->dmcdma_listeners, elem) {
        elem->callback(elem->context, addr);
    }
}

static inline void bus_event_detach_listener(bus_event_emitter_t *self, void *context)
{
    {
        _bus_event_listener_on_int_asserted_t *elem = NULL;

        list_foreach (&self->int_asserted_listeners, elem) {
            if (elem->context == context) {
                list_remove(&self->int_asserted_listeners, &elem->list_entry);
                FREE(elem);
            }
        }
    }
    {
        _bus_event_listener_on_int_acked_t *elem = NULL;

        list_foreach (&self->int_acked_listeners, elem) {
            if (elem->context == context) {
                list_remove(&self->int_acked_listeners, &elem->list_entry);
                FREE(elem);
            }
        }
    }
    {
        _bus_event_listener_on_oamdma_t *elem = NULL;

        list_foreach (&self->oamdma_listeners, elem) {
            if (elem->context == context) {
                list_remove(&self->oamdma_listeners, &elem->list_entry);
                FREE(elem);
            }
        }
    }
    {
        _bus_event_listener_on_dmcdma_t *elem = NULL;

        list_foreach (&self->dmcdma_listeners, elem) {
            if (elem->context == context) {
                list_remove(&self->dmcdma_listeners, &elem->list_entry);
                FREE(elem);
            }
        }
    }
}

static inline void bus_event_detach_listener_on_int_asserted(bus_event_emitter_t *self, void *context)
{
    _bus_event_listener_on_int_asserted_t *elem = NULL;
    list_foreach (&self->int_asserted_listeners, elem) {
        if (elem->context == context) {
            list_remove(&self->int_asserted_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
}
static inline void bus_event_detach_listener_on_int_acked(bus_event_emitter_t *self, void *context)
{
    _bus_event_listener_on_int_acked_t *elem = NULL;
    list_foreach (&self->int_acked_listeners, elem) {
        if (elem->context == context) {
            list_remove(&self->int_acked_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
}
static inline void bus_event_detach_listener_on_oamdma(bus_event_emitter_t *self, void *context)
{
    _bus_event_listener_on_oamdma_t *elem = NULL;
    list_foreach (&self->oamdma_listeners, elem) {
        if (elem->context == context) {
            list_remove(&self->oamdma_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
}
static inline void bus_event_detach_listener_on_dmcdma(bus_event_emitter_t *self, void *context)
{
    _bus_event_listener_on_dmcdma_t *elem = NULL;
    list_foreach (&self->dmcdma_listeners, elem) {
        if (elem->context == context) {
            list_remove(&self->dmcdma_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
}

static inline void bus_event_detach_all(bus_event_emitter_t *self)
{
    {
        _bus_event_listener_on_int_asserted_t *elem = NULL;

        list_foreach (&self->int_asserted_listeners, elem) {
            list_remove(&self->int_asserted_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
    {
        _bus_event_listener_on_int_acked_t *elem = NULL;

        list_foreach (&self->int_acked_listeners, elem) {
            list_remove(&self->int_acked_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
    {
        _bus_event_listener_on_oamdma_t *elem = NULL;

        list_foreach (&self->oamdma_listeners, elem) {
            list_remove(&self->oamdma_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
    {
        _bus_event_listener_on_dmcdma_t *elem = NULL;

        list_foreach (&self->dmcdma_listeners, elem) {
            list_remove(&self->dmcdma_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
}
