#pragma once
#include "utils.h"
typedef struct bus *bus_ref;
uint8_t bus_read(bus_ref self, uint16_t addr);
void bus_write(bus_ref self, uint16_t addr, uint8_t value);
