// generated file
#pragma once
#include "cpu.h"
#include "cpu_decode.h"
#include "utils.h"

<% profiles.each do |p| -%>
// [<%= p.name %>] OP:<%= p.op%> Mode:<%= p.mode %> Length:<%= p.length %> Memory:<%= p.memory %>
// Semantics:<%= p.code %>
<%= p.fname %>
{
<% p.gencode.each do |line| -%>
    <%= line %>
<% end -%>
}

<% end -%>
