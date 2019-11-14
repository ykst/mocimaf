#pragma once
#include "console.h"
#include <stdarg.h>

typedef enum console_mode {
    CONSOLE_MODE_NORMAL,
    CONSOLE_MODE_JUMP,
    CONSOLE_MODE_COMMAND,
} console_mode_t;

typedef struct char_buf {
    char str[256];
    int cursor;
} char_buf_t;

typedef struct console {
    core_ref shared_core;
    disas_ref shared_disas;
    debugger_ref shared_debugger;
    event_logger_driver_ref shared_event_logger;
    char_buf_t command_buf;
    char_buf_t jump_buf;
    console_mode_t mode;
    uint32_t base;
    uint32_t pc_phys;
    int view_height;
    int view_y;
    int middle_y;
    char display[256];
    bool follow;
    int cursor;
    int ram_offset_y;
    int prev_inspector_state;
    bool enable_event_log;
    console_pause_reason_t pause_reason;
} console_t;

static inline void char_buf_add(char_buf_t *self, int ch)
{
    if (self->cursor < 255) {
        self->str[self->cursor++] = ch;
        self->str[self->cursor] = '\0';
    }
}

static inline void char_buf_reset(char_buf_t *self)
{
    self->cursor = 0;
    self->str[0] = '\0';
}

static inline char char_buf_last(char_buf_t *self)
{
    char ret;

    if (self->cursor > 0) {
        ret = self->str[self->cursor - 1];
    } else {
        ret = '\0';
    }

    return ret;
}

static inline void char_buf_backspace(char_buf_t *self)
{

    if (self->cursor > 0) {
        self->str[--self->cursor] = '\0';
    }
}

static inline void console_display(console_ref self, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(self->display, 256, fmt, args);
    va_end(args);
}
