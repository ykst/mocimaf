#pragma once
#include "event_logger_driver_impl.h"

void event_logger_driver_read_start(event_logger_driver_ref self);

bool event_logger_driver_read(event_logger_driver_ref self, char *buf,
                              size_t len);

void event_logger_driver_trace_enable(event_logger_driver_ref self);
void event_logger_driver_trace_disable(event_logger_driver_ref self);
