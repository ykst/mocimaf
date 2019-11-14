#pragma once
#include "utils.h"

typedef struct counter {
    uint64_t cycles;
    int cycle_offset;
} counter_t;

typedef struct counters {
    counter_t cpu;
    counter_t apu;
    counter_t ppu;
} counters_t;
