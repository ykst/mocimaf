#include "logger.h"
#include "utils.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct logger {
    char *path;
    FILE *fp;
    loglevel_t level;
    char timestamp[26];
    bool is_filemode;
} logger_t;

static int _color_scheme[] = {
    [LOGLEVEL_ALWAYS] = 0, [LOGLEVEL_TRACE] = 0, [LOGLEVEL_DEBUG] = 33,
    [LOGLEVEL_WARN] = 35,  [LOGLEVEL_INFO] = 36, [LOGLEVEL_ERROR] = 31,
};

static const char *_update_timestamp(logger_ref self)
{
    time_t timer;
    struct tm *tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(self->timestamp, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    return self->timestamp;
}

void logger_destroy(logger_ref self)
{
    if (self) {
        if (self->path) {
            if (self->fp) {
                logger_append(self, LOGLEVEL_INFO,
                              "[%s] logfile closed (pid %d)\n",
                              _update_timestamp(self), getpid());
                fclose(self->fp);
                self->fp = NULL;
            }
            FREE(self->path);
        }

        FREE(self);
    }
}

const char *logger_get_path(logger_ref self)
{
    return self->path;
}

void logger_set_level(logger_ref self, loglevel_t level)
{
    self->level = level;
}

logger_ref _create(char *path, FILE *fp, loglevel_t level)
{
    logger_ref self = NULL;

    TALLOC(self);

    self->level = level;

    if (path) {
        self->is_filemode = true;
        self->path = path;
        self->fp = fp;

        logger_append(
            self, LOGLEVEL_ALWAYS,
            "------------------------------------------------------------"
            "-------------------\n[%s] logfile opened (pid %d)\n",
            _update_timestamp(self), getpid());
    }

    return self;
error:
    logger_destroy(self);
    return NULL;
}

logger_ref logger_create(loglevel_t level)
{
    return _create(NULL, NULL, level);
}

logger_ref logger_create_with_file(const char *maybe_path, loglevel_t level)
{
    FILE *fp = NULL;
    char *path = NULL;
    int fd = -1;

    if (maybe_path) {
        GUARD(fp = fopen(maybe_path, "a"));
        GUARD(path = strdup(maybe_path));
    } else {
        GUARD(path = strdup("/tmp/neslog_XXXXXX"));
        GUARD((fd = mkstemp(path)) != -1);
        GUARD(fp = fdopen(fd, "a"));
    }

    return _create(path, fp, level);
error:
    if (fp) {
        fclose(fp);
    } else if (fd >= 0) {
        close(fd);
    }

    FREE(path);

    return NULL;
}

void logger_append(logger_ref self, loglevel_t level, const char *fmt, ...)
{
    if (!self || level < self->level) {
        return;
    }

    FILE *fp = NULL;

    va_list args;
    va_start(args, fmt);

    if (self->is_filemode) {
        fp = self->fp;
        vfprintf(fp, fmt, args);
    } else {
        fp = (level == LOGLEVEL_ALWAYS) ? stdout : stderr;
        int color = _color_scheme[level];

        if (color) {
            fprintf(fp, "\x1b[%dm", color);
            vfprintf(fp, fmt, args);
            fputs("\x1b[0m", fp);
        } else {
            vfprintf(fp, fmt, args);
        }
    }

    va_end(args);
    fflush(fp);
}
