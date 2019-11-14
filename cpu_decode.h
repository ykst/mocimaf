#pragma once
#include "bus.h"
#include "cpu.h"
#include "utils.h"

static inline uint8_t cpu_read(cpu_t *self, uint16_t addr)
{
    return bus_read(self->shared_bus, addr);
}

void cpu_write(cpu_t *self, uint16_t addr, uint8_t value)
{
    bus_write(self->shared_bus, addr, value);
}

static inline uint8_t cpu_fetch(cpu_t *self)
{
    return cpu_read(self, self->reg.pc++);
}

static inline uint16_t cpu_fetch16(cpu_t *self)
{
    return (uint16_t)cpu_fetch(self) | ((uint16_t)cpu_fetch(self) << 8);
}

static inline uint8_t cpu_pull(cpu_t *self)
{
    bus_clock_cpu(self->shared_bus, 1);
    return self->map.stack[++self->reg.s];
}

static inline uint16_t cpu_pull16(cpu_t *self)
{
    return (uint16_t)cpu_pull(self) | ((uint16_t)cpu_pull(self) << 8);
}

static inline void cpu_push(cpu_t *self, uint8_t v)
{
    bus_clock_cpu(self->shared_bus, 1);
    self->map.stack[self->reg.s--] = v;
}

static inline void cpu_push16(cpu_t *self, uint16_t v)
{
    cpu_push(self, (uint8_t)(v >> 8));
    cpu_push(self, (uint8_t)v);
}

static inline void _emit_read_write_event(cpu_t *self, uint16_t addr,
                                          bool is_read)
{
    if (is_read) {
        cpu_event_emit_read(self->shared_emitter, self, addr, 1);
    } else {
        cpu_event_emit_write(self->shared_emitter, self, addr, 1);
    }
}

static inline uint8_t fetch_immediate(cpu_t *self)
{
    return cpu_fetch(self);
}

static inline uint16_t fetch_zero_page(cpu_t *self)
{
    uint8_t l = cpu_fetch(self);
    return (uint16_t)l;
}

static inline uint16_t fetch_zero_page_x(cpu_t *self)
{
    // read from address, add index register to it
    bus_clock_cpu(self->shared_bus, 1);
    return ((uint16_t)cpu_fetch(self) + self->reg.x) & 0xFF;
}

static inline uint16_t fetch_zero_page_y(cpu_t *self)
{
    // read from address, add index register to it
    bus_clock_cpu(self->shared_bus, 1);
    return ((uint16_t)cpu_fetch(self) + self->reg.y) & 0xFF;
}

static inline uint16_t fetch_absolute(cpu_t *self, bool is_read)
{
    uint16_t dst = cpu_fetch16(self);

    _emit_read_write_event(self, dst, is_read);

    return dst;
}

static inline uint16_t fetch_absolute_index(cpu_t *self, uint8_t index,
                                            bool is_read)
{
    uint16_t base = cpu_fetch16(self);
    uint16_t dst = base + index;
    uint16_t dummy_dst = (base & 0xFF00) | (dst & 0xFF);

    if (!is_read || (dummy_dst != dst)) {
        // dummy read on pch fix
        (void)cpu_read(self, dummy_dst);
    }

    _emit_read_write_event(self, dst, is_read);

    return dst;
}

static inline uint16_t fetch_absolute_x(cpu_t *self, bool is_read)
{
    return fetch_absolute_index(self, self->reg.x, is_read);
}

static inline uint16_t fetch_absolute_y(cpu_t *self, bool is_read)
{
    return fetch_absolute_index(self, self->reg.y, is_read);
}

static inline uint16_t fetch_pre_indirect_x(cpu_t *self, bool is_read)
{
    uint8_t addr = cpu_fetch(self);
    // read from the address, add X to it
    bus_clock_cpu(self->shared_bus, 1);

    uint16_t indirect = ((uint16_t)addr + self->reg.x) & 0xFF;
    uint8_t l = cpu_read(self, indirect);
    uint8_t h = cpu_read(self, (indirect + 1) & 0xFF);
    uint16_t dst = l | (h << 8);

    _emit_read_write_event(self, dst, is_read);

    return dst;
}

static inline uint16_t fetch_post_indirect_y(cpu_t *self, bool is_read)
{
    uint8_t indirect = cpu_fetch(self);
    uint8_t l = cpu_read(self, indirect);
    uint8_t h = cpu_read(self, (indirect + 1) & 0xFF);
    uint16_t base = l | (h << 8);
    uint16_t dst = base + self->reg.y;
    uint16_t dummy_dst = (base & 0xFF00) | (dst & 0xFF);

    if (!is_read || (dummy_dst != dst)) {
        (void)cpu_read(self, dummy_dst);
    }

    // if (self->shared_disas && is_read && dst >= 0x8000) {
    //     disas_mark_data_runtime(self->shared_disas, dst, 1);
    // }

    return dst;
}

static inline uint16_t fetch_relative(cpu_t *self)
{
    uint8_t l = cpu_fetch(self);
    int8_t offset = (int8_t)l;
    return self->reg.pc + offset;
}

static inline uint16_t fetch_indirect(cpu_t *self)
{
    uint16_t indirect = cpu_fetch16(self);
    uint8_t pcl = cpu_read(self, indirect);
    uint8_t pch;

    if ((indirect & 0xFF) == 0xFF) {
        // eratta: no pch fix
        pch = cpu_read(self, indirect & 0xFF00);
    } else {
        pch = cpu_read(self, indirect + 1);
    }

    return (uint16_t)(pch << 8) | (uint16_t)pcl;
}

static inline uint16_t cpu_read_vector(cpu_t *self, uint16_t addr)
{
    uint8_t l = cpu_read(self, addr);
    uint8_t h = cpu_read(self, addr + 1);
    return (uint16_t)l | ((uint16_t)h << 8);
}

static inline void cpu_decode_branch(cpu_t *self, uint16_t addr, bool cond)
{
    if (cond) {
        if ((self->reg.pc & 0xFF00) != (addr & 0xFF00)) {
            bus_clock_cpu(self->shared_bus, 2);

        } else {
            bus_clock_cpu(self->shared_bus, 1);
        }
        self->reg.pc = addr;
    }
}
