#include <curses.h>
#include "console_internal.h"
#include "console_input.h"

void console_handle_input(console_ref self, int ch)
{
    switch (self->mode) {
        case CONSOLE_MODE_COMMAND:
            <%= ErbalT.new({ :schemes => command_mode_inputs }).result('console_input_switchcase.template') %>
            break;
        case CONSOLE_MODE_NORMAL:
            <%= ErbalT.new({ :schemes => normal_mode_inputs }).result('console_input_switchcase.template') %>
            break;
        case CONSOLE_MODE_JUMP:
            <%= ErbalT.new({ :schemes => jump_mode_inputs }).result('console_input_switchcase.template') %>
            break;
    }
}
