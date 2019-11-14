#pragma once
#include "args.h"
#include "core.h"
#include "debugger.h"
#include "disas.h"
#include "event_logger_driver.h"
#include "utils.h"

typedef struct console *console_ref;

typedef enum console_pause_reason {
    CONSOLE_PAUSE_REASON_NONE = 0,
    CONSOLE_PAUSE_REASON_BREAKPOINT,
    CONSOLE_PAUSE_REASON_SCANLINE_BREAK,
    CONSOLE_PAUSE_REASON_INTERRUPTION_BREAK,
} console_pause_reason_t;

void console_destroy(console_ref self);
void console_global_finalize();
console_ref console_create(core_t *core, debugger_ref debugger, disas_ref disas,
                           event_logger_driver_ref event_logger);
int console_update(console_ref self);
void console_focus_pc(console_ref self);
void console_set_pause_reason(console_ref self, console_pause_reason_t reason);
