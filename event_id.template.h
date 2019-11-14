#pragma once
#include "utils.h"

typedef enum event_domain_id {
<% events.each do |e| -%>
    EVENT_DOMAIN_ID_<%= e.domain.upcase %>,
<% end -%>
} event_domain_id_t;

typedef enum event_id {
<% events.each do |e| -%>
<% e.actions.each do |a| -%>
    <%= a.id %>,
<% end -%>
<% end -%>
} event_id_t;

static inline const char *event_domain_id_show(event_domain_id_t id)
{
    switch (id) {
<% events.each do |e| -%>
    case EVENT_DOMAIN_ID_<%= e.domain.upcase %>:
        return "<%= e.domain.upcase %>";
<% end -%>
    }

    return "-";
}

static inline const char *event_id_show(event_id_t id)
{
    switch (id) {
<% events.each do |e| -%>
<% e.actions.each do |a| -%>
    case <%= a.id %>:
        return "<%= a.name.upcase %>";
<% end -%>
<% end -%>
    }

    return "-";
}
