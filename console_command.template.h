#pragma once
#include "utils.h"
#include "console_internal.h"

<% enums.each do |enum| %>
typedef enum <%= enum.decl %> {
<% enum.fields.each do |f| -%>
    <%= "#{ enum.decl.upcase }_#{ f.name.upcase }" %>,
<% end -%>
} <%= enum.decl %>_t;
<% end -%>

typedef enum console_command_type {
<% coms.select{ |com| !com.internal }.each do |com| -%>
    <%= com.enum %>,
<% end -%>
} console_command_type_t;

<% coms.each do |com| -%>
<%= com.impl_signature %>;
<% end -%>

void console_exec_command(console_ref self, const char *command, int argc, char **argv);
