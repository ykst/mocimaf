#pragma once
#include "utils.h"
#include "drivers.h"
#include "core.h"
<% includes.each do |inc| -%>
#include "<%= inc %>"
<% end -%>

// generated event detacher
<% events.each do |l| -%>
<% l.actions.each do |a| -%>
void <%= decl %>_detach_<%= l.domain %>_<%= a.name %>(<%= ref %> self, core_ref core);
<% end -%>
<% end -%>

// required implementations
void <%= decl %>_destroy(<%= ref %> self);
<%= ref %> <%= decl %>_create(core_ref core<%= expand_cargs(args) %>);
<% events.each do |l| -%>
<% l.actions.each do |a| -%>
void <%= decl %>_on_<%= l.domain %>_<%= a.name %>(<%= ref %> self<%= expand_cargs(a.args) %>);
<% end -%>
<% end -%>
