#pragma once
#include "console_command.h"
#include "console_internal.h"
#include "utils.h"

void console_handle_input(console_ref self, int ch);

void console_command_commit(console_ref self);

static inline void console_command_backto_normal_mode(console_ref self)
{
    self->mode = CONSOLE_MODE_NORMAL;
}

static inline void console_command_command_edit(console_ref self, int ch)
{
    switch (ch) {
    case 127: // DELETE
        char_buf_backspace(&self->command_buf);
        break;
    default:
        char_buf_add(&self->command_buf, ch);
        break;
    }
    console_display(self, ":%s", self->command_buf.str);
}

static inline void console_command_enter_command_mode(console_ref self)
{
    char_buf_reset(&self->command_buf);
    console_display(self, ":");
    self->mode = CONSOLE_MODE_COMMAND;
}

static inline void console_command_pause(console_ref self)
{
    core_toggle_pause(self->shared_core);

    self->follow = true;
}

static inline void console_command_cursor_reset(console_ref self, int position,
                                                int offset)
{
    self->base = disas_scroll_address(self->shared_disas, position, offset);
}

static inline void console_command_cursor_set(console_ref self, int position)
{
    self->cursor = position;
}

static inline void console_command_cursor_scroll(console_ref self, int scroll)
{
    self->base = disas_scroll_address(self->shared_disas, self->base, scroll);
}

static inline void console_command_cursor_offset(console_ref self, int offset)
{
    int next = self->cursor + offset;
    if (next < 0) {
        self->base = disas_scroll_address(self->shared_disas, self->base, next);
        self->cursor = 0;
    } else if (next >= self->view_height - 1) {
        self->base = disas_scroll_address(self->shared_disas, self->base,
                                          next - (self->view_height - 1));
        self->cursor = self->view_height - 1;
    } else {
        self->cursor = next;
    }
}

static inline void console_command_ram_offset(console_ref self, int offset)
{
    self->ram_offset_y += offset;
}

static inline void console_command_ram_set(console_ref self, int position)
{
    self->ram_offset_y = position;
}

static inline void console_command_step_instruction(console_ref self)
{
    debugger_set_step_mode(self->shared_debugger,
                           DEBUGGER_STEP_MODE_INSTRUCTION);

    console_display(self, "instruction step $%04x",
                    self->shared_core->cpu->reg.pc);
    self->follow = true;
}

static inline void console_command_step_scanline(console_ref self)
{
    debugger_set_step_mode(self->shared_debugger, DEBUGGER_STEP_MODE_SCANLINE);
    self->follow = true;
}

static inline void console_command_step_frame(console_ref self)
{
    debugger_set_step_mode(self->shared_debugger, DEBUGGER_STEP_MODE_FRAME);
    self->follow = true;
}

static inline void console_command_enter_jump_mode(console_ref self, int ch)
{
    self->mode = CONSOLE_MODE_JUMP;
    char_buf_reset(&self->jump_buf);
    char_buf_add(&self->jump_buf, ch);
}

static inline void console_command_jump_offset(console_ref self)
{
    int pos;

    if (self->jump_buf.str[0] == '$') {
        pos = strtoul(&self->jump_buf.str[1], NULL, 16);
    } else {
        pos = strtoul(self->jump_buf.str, NULL, 10);
    }

    self->cursor += pos;

    if (self->cursor >= self->view_height) {
        self->base = disas_scroll_address(self->shared_disas, pos,
                                          self->cursor - self->view_height + 1);
        self->cursor = self->view_height - 1;
    }

    self->mode = CONSOLE_MODE_NORMAL;
}

static inline void console_command_jump_commit(console_ref self)
{
    if (char_buf_last(&self->jump_buf) == 'g') {
        int pos;
        DUMPS(self->jump_buf.str);
        if (self->jump_buf.str[0] == '$') {
            pos = strtoul(&self->jump_buf.str[1], NULL, 16);
        } else {
            pos = strtoul(self->jump_buf.str, NULL, 10);
        }
        self->base = disas_scroll_address(self->shared_disas, pos, -1);
        self->base = disas_scroll_address(self->shared_disas, self->base, 1);
        self->mode = CONSOLE_MODE_NORMAL;
        self->cursor = 0;
        console_display(self, "jumped to $%05x", self->base);
    } else {
        char_buf_add(&self->jump_buf, 'g');
    }
}

static inline void console_command_jump_edit(console_ref self, int ch)
{
    if (ch == 127) {
        char_buf_backspace(&self->jump_buf);
    } else {
        char_buf_add(&self->jump_buf, ch);
    }
    console_display(self, "jump: %s", self->jump_buf.str);
}
