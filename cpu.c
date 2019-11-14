#include "cpu.h"
#include "apu.h"
#include "bus_internal.h"
#include "cpu_decode.h"
#include "cpu_decode_internal.h"
#include "interrupts.h"
#include "ppu.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void cpu_destroy(cpu_t *self)
{
    if (self) {
        FREE(self);
    }
}

cpu_t *cpu_create(cpu_event_emitter_t *emitter)
{
    cpu_t *self = NULL;

    TALLOC(self);
    GUARD(self->shared_emitter = emitter);

    return self;
error:
    cpu_destroy(self);
    return NULL;
}

int cpu_set_bus(cpu_t *self, bus_ref bus)
{
    GUARD(self);
    GUARD(!self->shared_bus);
    GUARD(self->shared_bus = bus);
    self->shared_counter = &self->shared_bus->counters.cpu;

    return SUCCESS;
error:
    return NG;
}

int cpu_cold_boot(cpu_t *self)
{
    GUARD(self->shared_bus);

#ifdef NESTEST
    self->reg.pc = 0xC000;
#else
    bus_assert_rst(self->shared_bus);
#endif

    self->reg.s = 0xFD;
#ifdef NESTEST
    self->reg.p = 0x24;
#else
    self->reg.p = 0x34;
#endif

    return SUCCESS;
error:
    return NG;
}

void cpu_soft_reset(cpu_t *self)
{
    bus_acknowledged_rst(self->shared_bus);
    self->reg.s -= 3;
    self->reg.int_disable = 1;
    self->reg.pc = cpu_read_vector(self, 0xFFFC);
    bus_clock_cpu(self->shared_bus, 5);
}

int cpu_simulate_interrupt(cpu_t *self)
{
    interrupts_t *ints = &self->shared_bus->ints;
    interruption_type_t handled = INTERRUTPION_TYPE_NONE;

    if (ints->interrupted) {
        // CAVEAT: If NMI is asserted during the first four ticks of a BRK
        // instruction, the BRK instruction will execute normally at first (PC
        // increments will occur and the status word will be pushed with the B
        // flag set), but execution will branch to the NMI vector instead of the
        // IRQ/BRK vector:
        if (ints->nmi) {
            uint16_t pc = self->reg.pc;

            cpu_push(self, (pc >> 8) & 0xFF);
            cpu_push(self, pc & 0xFF);
            bus_acknowledged_nmi(self->shared_bus);
            self->reg.p = (self->reg.p & 0xCF) | 0x20;
            cpu_push(self, self->reg.p);
            self->reg.int_disable = 1;
            self->reg.pc = cpu_read_vector(self, 0xFFFA);
            // PRINT("** NMI\n");
            bus_clock_cpu(self->shared_bus, 2);
            handled = INTERRUTPION_TYPE_NMI;
        } else if (ints->rst) {
            bus_soft_reset(self->shared_bus);
            handled = INTERRUTPION_TYPE_RST;
        } else if (ints->irq) {
            if (!self->reg.int_disable && !self->irq_poll_delay) {
                uint16_t pc = self->reg.pc;

                cpu_push(self, (pc >> 8) & 0xFF);
                cpu_push(self, pc & 0xFF);
                self->reg.p = (self->reg.p & 0xCF) | 0x20;
                cpu_push(self, self->reg.p);
                self->reg.int_disable = 1;
                self->reg.pc = cpu_read_vector(self, 0xFFFE);
                // PRINT("IRQ %d:%d.%d.%d\n", self->ppu->scanlines,
                // ints->frame,
                //        ints->dmc, ints->ext);
                bus_clock_cpu(self->shared_bus, 2);
                handled = INTERRUTPION_TYPE_IRQ;
                bus_acknowledged_irq(self->shared_bus);
            }
        }
    }

    if ((handled != INTERRUTPION_TYPE_NONE)) {
        cpu_event_emit_interrupt(self->shared_emitter, self, handled);
        // debugger_breakpoint_unhit(self->shared_debugger, self->reg.pc);
        // debugger_check_interruption_break(self->shared_debugger, handled);
        // self->paused = true;
        // self->on_interruption_break(self->on_interruption_break_ctx, self);
        //}
    }

#if 0
    if ((handled != INTERRUTPION_TYPE_NONE) && self->shared_debugger) {
        debugger_breakpoint_unhit(self->shared_debugger, self->reg.pc);
        debugger_check_interruption_break(self->shared_debugger, handled);
        // self->paused = true;
        // self->on_interruption_break(self->on_interruption_break_ctx, self);
        //}
    }
#endif

    self->irq_poll_delay = false;

    if (ints->reserve_nmi) {
        if (!ints->nmi) {
            // PRINT("RESERVED NMI\n");
            ints->nmi = true;
        }
        ints->reserve_nmi = false;
    }
    return SUCCESS;
}

#if 0
static void _error_dump(cpu_t *self)
{
    PRINT("******\n");
    PRINT("PC:%04x A:%02x  X:%02x  Y:%02x  P:%02x  SP:%02x\n", self->reg.pc - 1,
          self->reg.a, self->reg.x, self->reg.y, self->reg.p, self->reg.s);
    for (int i = 0; i < 256; ++i) {
        if ((i % 8) == 0) {
            PRINT("%04x: ", i + 256);
        }
        PRINT("%02x ", self->map.stack[i]);

        if ((i % 8) == 7) {
            PRINT("\n");
        }
    }
}

static inline void _log_current_op(cpu_t *self)
{
    if (self->shared_debugger) {
        if (self->reg.pc >= 0x8000) {
            uint8_t op = self->mapper->prg_base[self->mapper->on_prg_phys(
                self->mapper, self->reg.pc)];
            debugger_log_append(
                self->shared_debugger,
                (debugger_log_t){.type = EVENT_LOG_OP,
                                 .payload_op = {.pc = self->reg.pc, .op = op}

                });
        } else if (self->reg.pc < 0x800) {
            uint8_t op = self->map.ram[self->reg.pc];
            debugger_log_append(
                self->shared_debugger,
                (debugger_log_t){.type = EVENT_LOG_OP,
                                 .payload_op = {.pc = self->reg.pc, .op = op}

                });
        } else {
            debugger_log_append(
                self->shared_debugger,
                (debugger_log_t){.type = EVENT_LOG_OP_BUS,
                                 .payload_op = {.pc = self->reg.pc}

                });
        }
    }
}
#endif

// static inline void _disas_hint(cpu_t *self)
// {
//     if (self->shared_disas) {
//         disas_mark_op_runtime(self->shared_disas, self->reg.pc,
//                               self->op == OP_20_JSR);
//     }
// }

int cpu_clock(cpu_t *self)
{
    cpu_simulate_interrupt(self);
#ifdef NESTEST
    {
        uint8_t v = cpu_read(self, self->reg.pc);
        bus_clock_cpu(self->shared_bus, -1);
        PRINT("%04X  %02X ", self->reg.pc, v);
        PRINT("A:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%llu\n",
              self->reg.a, self->reg.x, self->reg.y, self->reg.p, self->reg.s,
              self->shared_counters->scanline->x,
              self->shared_counters->scanline->y,
              self->shared_counters->cpu_cycles + 7);
    }
#endif

    // maybe stopped by interruption break
    if (self->shared_bus->paused) {
        return SUCCESS;
    }

    //_log_current_op(self);
    cpu_event_emit_decode(self->shared_emitter, self, self->reg.pc);

    // maybe stopped by breakpoint
    if (self->shared_bus->paused) {
        return SUCCESS;
    }

    cpu_decode_internal(self);

    bus_clock_coprocessors(self->shared_bus);

    cpu_event_emit_step(self->shared_emitter, self);

    return SUCCESS;
    // error:
    //_error_dump(self);
    // return NG;
}
