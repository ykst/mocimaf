#pragma once

typedef struct logger *logger_ref;

typedef enum loglevel {
    LOGLEVEL_TRACE,
    LOGLEVEL_DEBUG,
    LOGLEVEL_INFO,
    LOGLEVEL_WARN,
    LOGLEVEL_ERROR,
    LOGLEVEL_ALWAYS,
} loglevel_t;

void logger_destroy(logger_ref self);
logger_ref logger_create(loglevel_t level);
logger_ref logger_create_with_file(const char *path, loglevel_t level);
void logger_append(logger_ref self, loglevel_t level, const char *fmt, ...);
const char *logger_get_path(logger_ref self);
void logger_set_level(logger_ref self, loglevel_t level);

#define printf XXX_printf_is_forbidden_use_TRACE_or_PRINT_instead_XXX
#define puts XXX_puts_is_forbidden_use_TRACE_or_PRINT_instead_XXX
