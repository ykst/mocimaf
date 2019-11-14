#pragma once
#include "utils.h"
<% includes.each do |inc| -%>
#include "<%= inc %>"
<% end -%>

<% actions.each do |a| -%>
<% a.enums.each do |enum| -%>

typedef enum <%= enum.decl %> {
<% enum.fields.each do |field| -%>
    <%= enum.decl.upcase %>_<%= field.upcase %>,
<% end -%>
} <%= enum.decl %>_t;
<% end -%>
typedef void (*<%= a.callback_decl %>)(void *context<%= expand_cargs(a.args) %>);
<% end -%>

typedef struct <%= listener_decl %> {
    void *context;
<% actions.each do |a| -%>
    // <%= a.desc %>
    <%= a.callback_decl %> on_<%= a.name %>;
<% end -%>
} <%= listener_decl %>_t;
