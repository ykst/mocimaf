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

#define CONSOLE_COMMAND_NUM (<%= coms.select{ |com| !com.internal }.size %>)

static console_command_scheme_t console_command_schemes[CONSOLE_COMMAND_NUM] = {
<% coms.select{ |com| !com.internal }.each do |com| -%>
    {
        .name = "<%= com.name %>",
<% if com.alias -%>
        .alias = "<%= com.alias %>",
<% end -%>
        .min_args = <%= com.args.select{ |a| a.default.nil? }.size %>,
        .max_args = <%= com.args.size %>,
        .type = <%= com.enum %>,
    },
<% end -%>
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
<% coms.select{ |com| !com.internal }.each do |com| -%>
                case <%= com.enum %>: {
<% com.argparses.each do |argparse| -%>
<%= indent(argparse.join("\n"), 5) %>
<% end -%>
                    <%= com.impl_call %>
                    break;
                }
<% end -%>
                }
                return;
            }
        }

        snprintf(self->display, 256, "unknown command: %s", command);
    }
}
