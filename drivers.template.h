#pragma once
#include "utils.h"
#include "core.h"
<% includes.each do |inc| -%>
#include "<%= inc %>"
<% end -%>

<% drivers.each do |d| -%>
// <%= d.desc %>
typedef struct <%= d.decl %> *<%= d.ref %>;
<%= d.ref %> <%= d.decl %>_plug(core_ref core<%= expand_cargs(d.args) %>);
void <%= d.decl %>_unplug(core_ref core, <%= d.ref %> self);
<% end -%>
