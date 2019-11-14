#pragma once
#include "apu.h"
#include "args.h"
#include "bus.h"
#include "counter.h"
#include "cpu_opcode.h"
#include "debugger.h"
#include "disas.h"
#include "ines.h"
#include "interrupts.h"
#include "joypad.h"
#include "mapper.h"
#include "ppu.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct cpu *cpu_ref;

#include "cpu_event_emitter.h"

typedef struct cpu {
    struct {
        uint16_t pc;
        uint8_t a;
        uint8_t x;
        uint8_t y;
        uint8_t s;
        union {
            struct {
                uint8_t carry : 1;
                uint8_t zero : 1;
                uint8_t int_disable : 1;
                uint8_t decimal : 1;
                uint8_t brk : 1;
                uint8_t : 1;
                uint8_t overflow : 1;
                uint8_t negative : 1;
            };
            uint8_t p;
        };
    } reg;

    // TODO: move to bus?
    struct {
        union {
            struct {
                uint8_t zero_page[0x100];
                uint8_t stack[0x100];
            };
            // 0x0000 ~ 0x0800 * 4 (mirrored)
            uint8_t ram[0x800];
        };
        // 0x6000 ~ 0x8000 may be battery back up'ed
        // uint8_t prg_ram[0x2000];
    } map;
    // uint8_t open_bus;
    // joypad_t joypads[2];

    // bool oamdma;
    // dmcdma_ctx dmcdma;

    const ines_t *shared_ines;

    bool irq_poll_delay;

    uint8_t last_op;

    cpu_event_emitter_t *shared_emitter;

    // void *on_step_ctx;
    // void (*on_step)(void *ctx, struct cpu *cpu);

    // void *on_breakpoint_ctx;
    // void (*on_breakpoint)(void *ctx, struct cpu *cpu);

    // void *on_memory_break_ctx;
    // void (*on_memory_break)(void *ctx, struct cpu *cpu);

    // void *on_interruption_break_ctx;
    // void (*on_interruption_break)(void *ctx, struct cpu *cpu);

    // bool paused;
    counter_t *shared_counter;

    // core
    // counters_t counters;
    // disas_ref shared_disas;
    // debugger_ref shared_debugger;
    // mapper_t *mapper;
    // ppu_t *ppu;
    // apu_t *apu;
    // interrupts_t *shared_ints;
    bus_ref shared_bus;
} cpu_t;

void cpu_init_reg(cpu_t *cpu);
int cpu_cold_boot(cpu_t *cpu);
void cpu_soft_reset(cpu_t *self);
cpu_t *cpu_create(cpu_event_emitter_t *emitter);
int cpu_set_bus(cpu_t *cpu, bus_ref bus);
int cpu_clock(cpu_t *cpu);
// void cpu_simulate_pause(cpu_t *cpu);
void cpu_destroy(cpu_t *self);
