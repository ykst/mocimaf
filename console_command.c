#include "console_command.h"
#include "string.h"

void console_command_quit(console_ref self)
{
    core_quit(self->shared_core);
}

void console_command_reset(console_ref self)
{
    core_reset(self->shared_core);
    snprintf(self->display, 256, "soft reset");
}

void console_command_break(console_ref self, int phys)
{
    bool after = debugger_breakpoint_toggle(self->shared_debugger, phys);

    snprintf(self->display, 256, "%s breakpoint at $%05x",
             after ? "set" : "unset", phys);
}

void console_command_break_int(console_ref self,
                               console_command_break_int_type_t type)
{
    char *hint = NULL;
    interruption_type_t target = INTERRUTPION_TYPE_NONE;

    switch (type) {
    case CONSOLE_COMMAND_BREAK_INT_TYPE_NMI:
        target = INTERRUTPION_TYPE_NMI;
        hint = "nmi";
        break;
    case CONSOLE_COMMAND_BREAK_INT_TYPE_RST:
        target = INTERRUTPION_TYPE_RST;
        hint = "rst";
        break;
    case CONSOLE_COMMAND_BREAK_INT_TYPE_IRQ:
        target = INTERRUTPION_TYPE_IRQ;
        hint = "irq";
        break;
    }
    bool after =
        debugger_interruption_breakpoint_toggle(self->shared_debugger, target);

    snprintf(self->display, 256, "toggle interruption breakpont: %s -> %d",
             hint, after);
}

void console_command_break_scanline(console_ref self, int line, int dot)
{
    debugger_set_scanline_break(self->shared_debugger, line, dot);

    snprintf(self->display, 256, "set scanline break: %d %d", line, dot);
}
void console_command_toggle_sound(console_ref self,
                                  console_command_toggle_sound_target_t target)
{
    bool after = false;
    char *hint = NULL;

    switch (target) {
    case CONSOLE_COMMAND_TOGGLE_SOUND_TARGET_PULSE1:
        // after = negate(&self->shared_inspector->channel_mute.pulse1);
        hint = "pulse1";
        break;
    case CONSOLE_COMMAND_TOGGLE_SOUND_TARGET_PULSE2:
        // after = negate(&self->shared_inspector->channel_mute.pulse2);
        hint = "pulse2";
        break;
    case CONSOLE_COMMAND_TOGGLE_SOUND_TARGET_TRIANGLE:
        // after = negate(&self->shared_inspector->channel_mute.triangle);
        hint = "triangle";
        break;
    case CONSOLE_COMMAND_TOGGLE_SOUND_TARGET_NOISE:
        // after = negate(&self->shared_inspector->channel_mute.noise);
        hint = "noise";
        break;
    case CONSOLE_COMMAND_TOGGLE_SOUND_TARGET_DMC:
        // after = negate(&self->shared_inspector->channel_mute.dmc);
        hint = "dmc";
        break;
    }

    snprintf(self->display, 256, "toggled %s: %s", hint, after ? "off" : "on");
}

void console_command_break_on_cursor(console_ref self)
{
    uint32_t pos =
        disas_scroll_address(self->shared_disas, self->base, self->cursor);
    console_command_break(self, pos);
}

void console_command_set(console_ref self, console_command_set_flag_t flag)
{
    switch (flag) {
    case CONSOLE_COMMAND_SET_FLAG_EVENT_TRACE:
        event_logger_driver_trace_enable(self->shared_event_logger);
        break;
    case CONSOLE_COMMAND_SET_FLAG_EVENT_LOG:
        self->enable_event_log = true;
        break;
    }
}

void console_command_unset(console_ref self, console_command_unset_flag_t flag)
{
    switch (flag) {
    case CONSOLE_COMMAND_UNSET_FLAG_EVENT_TRACE:
        event_logger_driver_trace_disable(self->shared_event_logger);
        break;
    case CONSOLE_COMMAND_UNSET_FLAG_EVENT_LOG:
        self->enable_event_log = false;
        break;
    }
}
