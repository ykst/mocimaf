#include "bus_internal.h"

uint8_t bus_read(bus_ref self, uint16_t addr)
{
    uint8_t ret;

    if (addr >= 0x8000) {
        ret = self->shared_mapper->on_prg_read(self->shared_mapper, addr);
    } else if (addr < 0x2000) {
        ret = self->shared_cpu->map.ram[addr & 0x7FF];
    } else if (addr < 0x4000) {
        bus_clock_coprocessors(self);
        ret = ppu_read_reg(self->shared_ppu, addr & 0x2007);
    } else if (addr < 0x4018) {
        switch (addr) {
        case 0x4014:
            ret = self->open_bus;
            break;
        case 0x4015:
            ret = apu_read_status(self->shared_apu);
            break;
        case 0x4016:
            ret = joypad_read(&self->joypads[0]) | (self->open_bus & 0xE0);
            self->open_bus = ret;
            break;
        case 0x4017:
            ret = joypad_read(&self->joypads[1]) | (self->open_bus & 0xE0);
            self->open_bus = ret;
            break;
        default:
            ret = self->open_bus;
            break;
        }
    } else if (addr < 0x6000) {
        // APU Test Register ~0x4020 and ext
        // NOP
        ret = self->open_bus;
    } else {
        if (self->shared_mapper->on_misc_read) {
            ret = self->shared_mapper->on_misc_read(self->shared_mapper, addr);
        } else {
            // PRINT("R: %04x\n", addr);
            ret = self->prg_ram[addr & 0x1FFF];
        }
    }

    bus_clock_cpu(self, 1);

    return ret;
}

void bus_write(bus_ref self, uint16_t addr, uint8_t value)
{
    if (addr >= 0x8000) {
        self->open_bus = value;
        self->shared_mapper->on_rom_write(self->shared_mapper, addr, value);
    } else if (addr < 0x2000) {
        self->shared_cpu->map.ram[addr & 0x7FF] = value;
    } else if (addr < 0x4000) {
        ppu_write_reg(self->shared_ppu, addr & 0x2007, value);
    } else if (addr < 0x4018) {
        switch (addr) {
        case 0x4014: {
            // DMA
            uint16_t from = (uint16_t)value << 8;
            uint8_t *to = self->shared_ppu->map.oam;
            bus_oamdma(self, from, to);
            break;
        }
        case 0x4016:
            joypad_write(&self->joypads[0], value);
            break;
        default:
            apu_write_reg(self->shared_apu, addr, value);
            break;
        }
    } else if (addr < 0x6000) {
        // APU Test Register ~0x4020 and ext
        // NOP
        if (self->shared_mapper->on_misc_write) {
            self->shared_mapper->on_misc_write(self->shared_mapper, addr,
                                               value);
        }
    } else {
        if (self->shared_mapper->on_misc_write) {
            self->shared_mapper->on_misc_write(self->shared_mapper, addr,
                                               value);
        } else {
            self->prg_ram[addr & 0x1FFF] = value;
        }
        // PRINT("W: %04x <- %02x\n", addr, value);
    }

    bus_clock_cpu(self, 1);
}

void bus_destroy(bus_ref self)
{
    if (self) {
        FREE(self);
    }
}

bus_ref bus_create(bus_event_emitter_t *emitter, mapper_t *mapper, cpu_t *cpu,
                   apu_t *apu, ppu_t *ppu, input_ref input)
{
    bus_ref self = NULL;

    TALLOC(self);
    GUARD(self->shared_mapper = mapper);
    GUARD(self->shared_ppu = ppu);
    GUARD(self->shared_apu = apu);
    GUARD(self->shared_cpu = cpu);
    GUARD(self->shared_input = input);
    GUARD(self->shared_emitter = emitter);
    GUARD(mapper_set_bus(self->shared_mapper, self));
    GUARD(ppu_set_bus(self->shared_ppu, self));
    GUARD(apu_set_bus(self->shared_apu, self));
    GUARD(cpu_set_bus(self->shared_cpu, self));
    GUARD(input_set_bus(self->shared_input, self));

    self->prg_size = self->shared_mapper->prg_size;

    // if (debugger) {
    //    self->shared_debugger = debugger;
    //    GUARD(debugger_set_bus(self->shared_debugger, self));
    //}

    return self;
error:
    bus_destroy(self);
    return NULL;
}
