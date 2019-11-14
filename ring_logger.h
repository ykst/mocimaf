#pragma once
#include "counter.h"
#include "utils.h"

typedef struct ring_logger *ring_logger_ref;
void ring_logger_destroy(ring_logger_ref self);
ring_logger_ref ring_logger_create(size_t cnt, size_t elem_size);
void ring_logger_append(ring_logger_ref self, void *buf);
void ring_logger_init_cursor(ring_logger_ref self);
const void *ring_logger_read(ring_logger_ref self);