#include "ring_logger.h"
#include <string.h>

typedef struct ring_logger {
    void *buf;
    size_t cnt;
    size_t elem_size;
    size_t size;
    int log_head;
    int log_tail;
    int log_cursor;
} ring_logger_t;

void ring_logger_destroy(ring_logger_ref self)
{
    if (self) {
        FREE(self->buf);
        FREE(self);
    }
}

ring_logger_ref ring_logger_create(size_t cnt, size_t elem_size)
{
    ring_logger_ref self = NULL;

    TALLOC(self);
    GUARD(self->cnt = cnt);
    GUARD(self->elem_size = elem_size);
    self->size = self->cnt * self->elem_size;
    TALLOCS(self->buf, self->size);

    return self;
error:
    return NULL;
}

void ring_logger_append(ring_logger_ref self, void *buf)
{
    void *e = &self->buf[self->log_head];
    //scanline_t *scanline = self->shared_counters->scanline;
    //e->log = log;
    //e->scanline = scanline->y;
    //e->dot = scanline->x;
    memcpy(e, buf, self->elem_size);

    self->log_head = (self->log_head + self->elem_size) % self->size;

    if (self->log_head == self->log_tail) {
        self->log_tail = (self->log_tail + self->elem_size) % self->size;
    }
}

void ring_logger_init_cursor(ring_logger_ref self)
{
    self->log_cursor =
        (self->log_head + self->size - self->elem_size) % self->size;
}

const void *ring_logger_read(ring_logger_ref self)
{
    if (self->log_tail == self->log_cursor ||
        self->log_head == self->log_tail) {
        return NULL;
    }

    void *ret = &self->buf[self->log_cursor];
    self->log_cursor =
        (self->log_cursor + self->size - self->elem_size) % self->size;

    return ret;
}
