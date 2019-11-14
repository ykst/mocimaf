#pragma once
#include "event_id.h"
#include "<%= listener_decl %>.h"
#include "list.h"
<% if domain != "logger" -%>
#include "logger_event_emitter.h"
<% end -%>

<% actions.each do |a| -%>
typedef struct <%= a.elem_decl %> {
    list_t list_entry;
    void *context;
    <%= a.callback_decl %> callback;
} <%= a.elem_decl %>_t;
<% end -%>

typedef struct <%= emitter_decl %> {
<% actions.each do |a| -%>
    list_head_t <%= a.name %>_listeners;
<% end -%>
<% if domain != "logger" -%>
    logger_event_emitter_t *shared_logger_emitter;
<% end -%>
} <%= emitter_decl %>_t;

static inline int <%= domain %>_event_attach_listener(<%= emitter_decl %>_t *self, <%= listener_decl %>_t *listener)
{
<% actions.each do |a| -%>
    if (listener->on_<%= a.name %>) {
        <%= a.elem_decl %>_t *elem = NULL;
        TALLOC(elem);
        elem->context = listener->context;
        elem->callback = listener->on_<%= a.name %>;
        list_append(&self-><%= a.name %>_listeners, &elem->list_entry);
    }
<% end -%>
    return SUCCESS;
error:
    return NG;
}

<% if domain != "logger" -%>
static inline void <%= domain %>_event_set_logger_emitter(<%= emitter_decl %>_t *self, logger_event_emitter_t *logger_emitter)
{
    self->shared_logger_emitter = logger_emitter;
}
<% end -%>

<% actions.each do |a| -%>
// <%= a.desc %>
static inline void <%= domain %>_event_emit_<%= a.name %>(<%= emitter_decl %>_t *self<%= expand_cargs(a.args) %>)
{
<% if domain != "logger" and a.logging -%>
    if (self->shared_logger_emitter) {
        logger_event_emit_event_trace(self->shared_logger_emitter, EVENT_DOMAIN_ID_<%= domain.upcase %>, <%= a.id %>);
    }

<% end -%>
    <%= a.elem_decl %>_t *elem = NULL;
    list_foreach (&self-><%= a.name %>_listeners, elem) {
        elem->callback(elem->context<%= expand_cparams(a.args) %>);
    }
}
<% end -%>

static inline void <%= domain %>_event_detach_listener(<%= emitter_decl %>_t *self, void *context)
{
<% actions.each do |a| -%>
    {
        <%= a.elem_decl %>_t *elem = NULL;

        list_foreach (&self-><%= a.name %>_listeners, elem) {
            if (elem->context == context) {
                list_remove(&self-><%= a.name %>_listeners, &elem->list_entry);
                FREE(elem);
            }
        }
    }
<% end -%>
}

<% actions.each do |a| -%>
static inline void <%= domain %>_event_detach_listener_on_<%= a.name %>(<%= emitter_decl %>_t *self, void *context)
{
    <%= a.elem_decl %>_t *elem = NULL;
    list_foreach (&self-><%= a.name %>_listeners, elem) {
        if (elem->context == context) {
            list_remove(&self-><%= a.name %>_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
}
<% end -%>

static inline void <%= domain %>_event_detach_all(<%= emitter_decl %>_t *self)
{
<% actions.each do |a| -%>
    {
        <%= a.elem_decl %>_t *elem = NULL;

        list_foreach (&self-><%= a.name %>_listeners, elem) {
            list_remove(&self-><%= a.name %>_listeners, &elem->list_entry);
            FREE(elem);
        }
    }
<% end -%>
}
