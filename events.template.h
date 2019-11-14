#pragma once
#include "utils.h"
#include "event_id.h"
<% events.each do |e| -%>
#include "<%= e.domain %>_event_emitter.h"
<% end -%>

typedef struct events *events_ref;

typedef struct events {
<% events.each do |e| -%>
    <%= e.domain %>_event_emitter_t <%= e.domain %>;
<% end -%>
} events_t;

static inline void events_detach_all(events_ref self)
{
<% events.each do |e| -%>
    <%= e.domain %>_event_detach_all(&self-><%= e.domain %>);
<% end -%>
}
