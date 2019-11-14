// *** WARNING ***
// This file is generated by genconsole.rb
// Keep untouched or ruin the abstraction
#include "utils.h"
#include "console_internal.h"
#include "console_command.h"

typedef struct console_command_scheme {
    const char *name;
    const char *alias;
    int min_args;
    int max_args;
    console_command_type_t type;
} console_command_scheme_t;

#define CONSOLE_COMMAND_NUM (8)

static console_command_scheme_t console_command_schemes[CONSOLE_COMMAND_NUM] = {
    {
        .name = "quit",
        .alias = "q",
        .min_args = 0,
        .max_args = 0,
        .type = CONSOLE_COMMAND_TYPE_QUIT,
    },
    {
        .name = "reset",
        .alias = "r",
        .min_args = 0,
        .max_args = 0,
        .type = CONSOLE_COMMAND_TYPE_RESET,
    },
    {
        .name = "break",
        .alias = "b",
        .min_args = 0,
        .max_args = 1,
        .type = CONSOLE_COMMAND_TYPE_BREAK,
    },
    {
        .name = "break_int",
        .alias = "bi",
        .min_args = 1,
        .max_args = 1,
        .type = CONSOLE_COMMAND_TYPE_BREAK_INT,
    },
    {
        .name = "break_scanline",
        .alias = "bs",
        .min_args = 1,
        .max_args = 2,
        .type = CONSOLE_COMMAND_TYPE_BREAK_SCANLINE,
    },
    {
        .name = "toggle_sound",
        .alias = "ts",
        .min_args = 1,
        .max_args = 1,
        .type = CONSOLE_COMMAND_TYPE_TOGGLE_SOUND,
    },
    {
        .name = "set",
        .alias = "s",
        .min_args = 1,
        .max_args = 1,
        .type = CONSOLE_COMMAND_TYPE_SET,
    },
    {
        .name = "unset",
        .alias = "u",
        .min_args = 1,
        .max_args = 1,
        .type = CONSOLE_COMMAND_TYPE_UNSET,
    },
};

void console_exec_command(console_ref self, const char *command, int argc, char **argv)
{
    if (command && strnlen(command, 256) > 0) {
        for (int i = 0; i < CONSOLE_COMMAND_NUM; ++i) {
            console_command_scheme_t *c = &console_command_schemes[i];
            if ((c->alias && (strncmp(command, c->alias, 256) == 0)) ||
                strncmp(command, c->name, 256) == 0) {

                if (argc < c->min_args || argc > c->max_args) {
                    snprintf(self->display, 256,
                             "wrong arguments for '%s': expecting min %d, max "
                             "%d arguments",
                             c->name, c->min_args, c->max_args);
                    return;
                }
                switch (c->type) {
                case CONSOLE_COMMAND_TYPE_QUIT: {
                    console_command_quit(self);
                    break;
                }
                case CONSOLE_COMMAND_TYPE_RESET: {
                    console_command_reset(self);
                    break;
                }
                case CONSOLE_COMMAND_TYPE_BREAK: {
                    int phys = (argc <= 0) ? self->pc_phys : ((argv[0][0] == '$') ? strtoul(&argv[0][1], NULL, 16) : atoi(argv[0]));
                    console_command_break(self, phys);
                    break;
                }
                case CONSOLE_COMMAND_TYPE_BREAK_INT: {
                    console_command_break_int_type_t type;
                    if (strncmp(argv[0], "n", 256) == 0 || strncmp(argv[0], "nmi", 256) == 0) { type = CONSOLE_COMMAND_BREAK_INT_TYPE_NMI; }
                    else if (strncmp(argv[0], "i", 256) == 0 || strncmp(argv[0], "irq", 256) == 0) { type = CONSOLE_COMMAND_BREAK_INT_TYPE_IRQ; }
                    else if (strncmp(argv[0], "r", 256) == 0 || strncmp(argv[0], "rst", 256) == 0) { type = CONSOLE_COMMAND_BREAK_INT_TYPE_RST; }
                    else { console_display(self, "unknown %s subcommand: %s", "break_int", argv[0]); return; }
                    console_command_break_int(self, type);
                    break;
                }
                case CONSOLE_COMMAND_TYPE_BREAK_SCANLINE: {
                    int line = ((argv[0][0] == '$') ? strtoul(&argv[0][1], NULL, 16) : atoi(argv[0]));
                    int dot = (argc <= 1) ? 0 : ((argv[1][0] == '$') ? strtoul(&argv[1][1], NULL, 16) : atoi(argv[1]));
                    console_command_break_scanline(self, line, dot);
                    break;
                }
                case CONSOLE_COMMAND_TYPE_TOGGLE_SOUND: {
                    console_command_toggle_sound_target_t target;
                    if (strncmp(argv[0], "n", 256) == 0 || strncmp(argv[0], "noise", 256) == 0) { target = CONSOLE_COMMAND_TOGGLE_SOUND_TARGET_NOISE; }
                    else if (strncmp(argv[0], "1", 256) == 0 || strncmp(argv[0], "pulse1", 256) == 0) { target = CONSOLE_COMMAND_TOGGLE_SOUND_TARGET_PULSE1; }
                    else if (strncmp(argv[0], "2", 256) == 0 || strncmp(argv[0], "pulse2", 256) == 0) { target = CONSOLE_COMMAND_TOGGLE_SOUND_TARGET_PULSE2; }
                    else if (strncmp(argv[0], "d", 256) == 0 || strncmp(argv[0], "dmc", 256) == 0) { target = CONSOLE_COMMAND_TOGGLE_SOUND_TARGET_DMC; }
                    else if (strncmp(argv[0], "t", 256) == 0 || strncmp(argv[0], "triangle", 256) == 0) { target = CONSOLE_COMMAND_TOGGLE_SOUND_TARGET_TRIANGLE; }
                    else { console_display(self, "unknown %s subcommand: %s", "toggle_sound", argv[0]); return; }
                    console_command_toggle_sound(self, target);
                    break;
                }
                case CONSOLE_COMMAND_TYPE_SET: {
                    console_command_set_flag_t flag;
                    if (strncmp(argv[0], "et", 256) == 0 || strncmp(argv[0], "event_trace", 256) == 0) { flag = CONSOLE_COMMAND_SET_FLAG_EVENT_TRACE; }
                    else if (strncmp(argv[0], "el", 256) == 0 || strncmp(argv[0], "event_log", 256) == 0) { flag = CONSOLE_COMMAND_SET_FLAG_EVENT_LOG; }
                    else { console_display(self, "unknown %s subcommand: %s", "set", argv[0]); return; }
                    console_command_set(self, flag);
                    break;
                }
                case CONSOLE_COMMAND_TYPE_UNSET: {
                    console_command_unset_flag_t flag;
                    if (strncmp(argv[0], "et", 256) == 0 || strncmp(argv[0], "event_trace", 256) == 0) { flag = CONSOLE_COMMAND_UNSET_FLAG_EVENT_TRACE; }
                    else if (strncmp(argv[0], "el", 256) == 0 || strncmp(argv[0], "event_log", 256) == 0) { flag = CONSOLE_COMMAND_UNSET_FLAG_EVENT_LOG; }
                    else { console_display(self, "unknown %s subcommand: %s", "unset", argv[0]); return; }
                    console_command_unset(self, flag);
                    break;
                }
                }
                return;
            }
        }

        snprintf(self->display, 256, "unknown command: %s", command);
    }
}