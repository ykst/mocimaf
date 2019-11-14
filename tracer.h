#pragma once
#include "core.h"
#include "md5.h"
#include "utils.h"
typedef struct tracer *tracer_ref;

void tracer_destroy(tracer_ref self);
tracer_ref tracer_create(void);

void tracer_start(tracer_ref self);
int tracer_flush(tracer_ref self, core_t *core);
void tracer_cpu_trace(tracer_ref self, cpu_t *cpu);
void tracer_apu_trace(tracer_ref self, uint8_t pulse, uint8_t tnd);
void tracer_scanline_trace(tracer_ref self, int line, const uint16_t *dots);

void tracer_verbose_flush_scanline(tracer_ref self, ppu_t *apu);
void tracer_verbose_flush_apu(tracer_ref self, apu_t *apu);
void tracer_verbose_flush_cpu(tracer_ref self, cpu_t *cpu);
