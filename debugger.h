#pragma once
#include "bus.h"
#include "counter.h"
#include "inspector.h"
#include "mapper.h"
#include "ppu.h"
#include "utils.h"

typedef struct debugger *debugger_ref;

typedef enum debugger_step_mode {
    DEBUGGER_STEP_MODE_NONE = 0,
    DEBUGGER_STEP_MODE_FRAME,
    DEBUGGER_STEP_MODE_SCANLINE,
    DEBUGGER_STEP_MODE_INSTRUCTION,
} debugger_step_mode_t;

bool debugger_breakpoint_toggle(debugger_ref self, uint32_t phys);
bool debugger_check_breakpoint_hit(debugger_ref self, uint16_t virt);
void debugger_breakpoint_unhit(debugger_ref self, uint16_t virt);
bool debugger_interruption_breakpoint_toggle(debugger_ref self,
                                             interruption_type_t type);
bool debugger_check_interruption_break(debugger_ref self,
                                       interruption_type_t type);
int debugger_set_scanline_break(debugger_ref self, int line, int dot);
bool debugger_check_scanline_before(debugger_ref self, int x, int y);
bool debugger_check_scanline_after(debugger_ref self, int x, int y);
void debugger_reset_scanline_break(debugger_ref self);
void debugger_destroy(debugger_ref self);
debugger_ref debugger_create(bus_ref bus);
bool debugger_is_breakpoint(debugger_ref self, uint32_t phys);

debugger_step_mode_t debugger_get_step_mode(debugger_ref self);
void debugger_set_step_mode(debugger_ref self, debugger_step_mode_t step_mode);
