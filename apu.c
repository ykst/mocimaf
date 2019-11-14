#include "apu.h"
#include "bus_internal.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t triangle_sequence[32] = {15, 14, 13, 12, 11, 10, 9,  8,  7,  6, 5,
                                 4,  3,  2,  1,  0,  0,  1,  2,  3,  4, 5,
                                 6,  7,  8,  9,  10, 11, 12, 13, 14, 15};

uint32_t frame_step_4[4] = {7457, 7458, 7458, 7458};
uint32_t frame_step_5[5] = {7457, 7458, 7458, 7458, 7452};
uint8_t length_lut[32] = {10, 254, 20,  2,  40, 4,  80, 6,  160, 8,  60,
                          10, 14,  12,  26, 14, 12, 16, 24, 18,  48, 20,
                          96, 22,  192, 24, 72, 26, 16, 28, 32,  30};
uint16_t noise_tval_lut[16] = {4,   8,   16,  32,  64,  96,   128,  160,
                               202, 254, 380, 508, 762, 1016, 2034, 4068};
uint16_t dmc_tval_lut[16] = {428, 380, 340, 320, 286, 254, 226, 214,
                             190, 160, 142, 128, 106, 84,  72,  54};

static inline void apu_update_frame_cycles(apu_t *self, int delta_cpu_cycles,
                                           bool *out_qframe, bool *out_hframe)
{
    uint32_t next_cycles = self->cycles + delta_cpu_cycles;

    if (next_cycles >= self->next_qframe_cycles) {
        next_cycles -= self->next_qframe_cycles;
        if (self->reg.frame_counter.mode) {
            self->frame_phase = (self->frame_phase + 1) % 5;
            // 5-step
            self->next_qframe_cycles = frame_step_5[self->frame_phase];
            if (self->frame_phase != 4) {
                *out_qframe = true;
            }
            if (self->frame_phase == 2 || self->frame_phase == 0) {
                *out_hframe = true;
            }
        } else {
            self->frame_phase = (self->frame_phase + 1) % 4;
            // 4-step
            self->next_qframe_cycles = frame_step_4[self->frame_phase];
            *out_qframe = true;
            if (self->frame_phase == 4) {
                *out_hframe = true;
            } else if (self->frame_phase == 0) {
                *out_hframe = true;
                if (!self->reg.frame_counter.int_disable) {
                    bus_assert_frame_irq(self->shared_bus);
                }
            }
        }
    }

    self->cycles = next_cycles;
}

uint8_t pulse_duty_lut[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1},
};

int apu_clock(apu_t *self)
{
    int32_t delta_cycles = self->shared_counter->cycle_offset;
    self->shared_counter->cycle_offset = 0;

    while ((delta_cycles -= 1) >= 0) {
        bool clock_qframe = false;
        bool clock_hframe = false;

        uint8_t pulse_d = 0;
        uint8_t tnd_d = 0;
        // float tnd_f = 0;

        apu_update_frame_cycles(self, 1, &clock_qframe, &clock_hframe);

        { // pulse1
            // linear counter clock
            if (clock_qframe) {
                if (self->pulse1.start) {
                    self->pulse1.start = false;
                    self->pulse1.decay = 15;
                    self->pulse1.divider = self->reg.pulse1.envelope.volume;
                } else {
                    if (self->pulse1.divider == 0) {
                        self->pulse1.divider = self->reg.pulse1.envelope.volume;

                        // envelope decay clock
                        if (self->pulse1.decay != 0) {
                            self->pulse1.decay -= 1;
                        } else if (self->reg.pulse1.envelope.loop) {
                            self->pulse1.decay = 15;
                        }
                    } else {
                        self->pulse1.divider -= 1;
                    }
                }
            }

            // length/sweep counter clock
            if (clock_hframe) {
                if (!self->reg.pulse1.envelope.loop &&
                    self->pulse1.length_counter != 0) {
                    self->pulse1.length_counter -= 1;
                }
                if (self->pulse1.sweep_divider == 0) {
                    int32_t adjust =
                        self->pulse1.tval >> (self->reg.pulse1.sweep.shift);
                    if (self->reg.pulse1.sweep.negate) {
                        adjust = -adjust - 1;
                    }
                    int32_t target_period = self->pulse1.tval + adjust;
                    self->pulse1.sweep_mute = target_period >= 0x7FF;

                    if (self->reg.pulse1.sweep.enabled &&
                        !self->pulse1.sweep_mute) {
                        self->pulse1.tval = target_period;
                    }
                    self->pulse1.sweep_divider = self->reg.pulse1.sweep.period;
                    self->pulse1.sweep_reload = false;
                } else {
                    if (self->pulse1.sweep_reload) {
                        self->pulse1.sweep_divider =
                            self->reg.pulse1.sweep.period;
                        self->pulse1.sweep_reload = false;
                    } else {
                        self->pulse1.sweep_divider -= 1;
                    }
                }
            }

            if (self->reg.pulse1.envelope.constant) {
                self->pulse1.volume = self->reg.pulse1.envelope.volume;
            } else {
                self->pulse1.volume = self->pulse1.decay;
            }

            // timer clock
            if (--self->pulse1.tval_counter <= 0) {
                self->pulse1.tval_counter = self->pulse1.tval * 2;

                if (self->pulse1.length_counter != 0) {
                    self->pulse1.sequence = (self->pulse1.sequence + 1) % 8;
                    // TRACE("%d\n", self->pulse1.sequence);
                }
            }

            if (pulse_duty_lut[self->reg.pulse1.envelope.duty]
                              [self->pulse1.sequence] &&
                !self->pulse1.sweep_mute && self->pulse1.length_counter != 0 &&
                self->pulse1.tval >= 8 && !self->channel_mute.pulse1) {
                pulse_d += self->pulse1.volume;
            }
        }

        { // pulse2
            // linear counter clock
            if (clock_qframe) {
                if (self->pulse2.start) {
                    self->pulse2.start = false;
                    self->pulse2.decay = 15;
                    self->pulse2.divider = self->reg.pulse2.envelope.volume;
                } else {
                    if (self->pulse2.divider == 0) {
                        self->pulse2.divider = self->reg.pulse2.envelope.volume;

                        // envelope decay clock
                        if (self->pulse2.decay != 0) {
                            self->pulse2.decay -= 1;
                        } else if (self->reg.pulse2.envelope.loop) {
                            self->pulse2.decay = 15;
                        }
                    } else {
                        self->pulse2.divider -= 1;
                    }
                }
            }

            // length/sweep counter clock
            if (clock_hframe) {
                if (!self->reg.pulse2.envelope.loop &&
                    self->pulse2.length_counter != 0) {
                    self->pulse2.length_counter -= 1;
                }
                if (self->pulse2.sweep_divider == 0) {
                    int32_t adjust =
                        self->pulse2.tval >> (self->reg.pulse2.sweep.shift);
                    if (self->reg.pulse2.sweep.negate) {
                        adjust = -adjust;
                    }
                    int32_t target_period = self->pulse2.tval + adjust;
                    self->pulse2.sweep_mute = target_period >= 0x7FF;
                    // if (self->pulse2.sweep_mute) {
                    //     TRACE("SWEEP MUTE\n");
                    // }

                    if (self->reg.pulse2.sweep.enabled &&
                        !self->pulse2.sweep_mute) {
                        self->pulse2.tval = target_period;
                    }

                    self->pulse2.sweep_reload = false;
                } else {
                    if (self->pulse2.sweep_reload) {
                        self->pulse2.sweep_divider =
                            self->reg.pulse2.sweep.period;
                        self->pulse2.sweep_reload = false;
                    } else {
                        self->pulse2.sweep_divider -= 1;
                    }
                }
            }

            if (self->reg.pulse2.envelope.constant) {
                self->pulse2.volume = self->reg.pulse2.envelope.volume;
            } else {
                self->pulse2.volume = self->pulse2.decay;
            }

            // timer clock
            if (--self->pulse2.tval_counter <= 0) {
                self->pulse2.tval_counter = self->pulse2.tval * 2;

                if (self->pulse2.length_counter != 0) {
                    self->pulse2.sequence = (self->pulse2.sequence + 1) % 8;
                    // TRACE("%d\n", self->pulse2.sequence);
                }
            }

            if (pulse_duty_lut[self->reg.pulse2.envelope.duty]
                              [self->pulse2.sequence] &&
                !self->pulse2.sweep_mute && self->pulse2.length_counter != 0 &&
                self->pulse2.tval >= 8 && !self->channel_mute.pulse2) {
                pulse_d += self->pulse2.volume;
            }
        }

        { //  noise
            // linear counter clock
            if (clock_qframe) {
                if (self->noise.start) {
                    self->noise.start = false;
                    self->noise.decay = 15;
                    self->noise.divider = self->reg.noise.envelope.volume;
                } else {
                    if (self->noise.divider == 0) {
                        self->noise.divider = self->reg.noise.envelope.volume;

                        // envelope decay clock
                        if (self->noise.decay != 0) {
                            self->noise.decay -= 1;
                        } else if (self->reg.noise.envelope.loop) {
                            self->noise.decay = 15;
                        }
                    } else {
                        self->noise.divider -= 1;
                    }
                }
            }

            // length/sweep counter clock
            if (clock_hframe) {
                if (!self->reg.noise.envelope.loop &&
                    self->noise.length_counter != 0) {
                    self->noise.length_counter -= 1;
                }
            }

            if (self->reg.noise.envelope.constant) {
                self->noise.volume = self->reg.noise.envelope.volume;
            } else {
                self->noise.volume = self->noise.decay;
            }

            // timer clock
            if (--self->noise.tval_counter <= 0) {
                self->noise.tval_counter = self->noise.tval;

                // if (self->noise.length_counter != 0) {
                uint16_t feedback =
                    ((self->noise.shift & 1) ^
                     ((self->noise.shift >> (self->reg.noise.mode ? 6 : 1)) &
                      1))
                    << 14;
                self->noise.shift = (self->noise.shift >> 1) | feedback;
                //}
            }

            if ((self->noise.length_counter != 0) && !(self->noise.shift & 1) &&
                !self->channel_mute.noise) {
                tnd_d += self->noise.volume * 2;
                // tnd_f += 0.00494f * self->noise.volume;
            }
        }

        { // triangle
            // linear counter clock
            if (clock_qframe) {
                if (self->triangle.reload) {
                    self->triangle.linear_counter = self->triangle.linear;
                } else if (self->triangle.linear_counter != 0) {
                    self->triangle.linear_counter -= 1;
                }
                if (!self->reg.triangle.control) {
                    self->triangle.reload = false;
                }
            }

            // length counter clock
            if (clock_hframe && !self->reg.triangle.control &&
                self->triangle.length_counter != 0) {
                self->triangle.length_counter -= 1;
            }

            // timer clock
            if (--self->triangle.tval_counter <= 0) {
                self->triangle.tval_counter = self->triangle.tval;
                if (self->triangle.length_counter != 0 &&
                    self->triangle.linear_counter != 0
                    // XXX: force to surpress ultra sonic noise
                    && self->triangle.tval > 7) {
                    self->triangle.sequence =
                        (self->triangle.sequence + 1) % 32;
                }
            }

            // Silencing the triangle channel merely halts it. It will
            // continue to output its last value, rather than 0. There is no
            // way to reset the triangle channel's phase.
            if (!self->channel_mute.triangle) {
                tnd_d += triangle_sequence[self->triangle.sequence] * 3;
            }
            // tnd_f += 0.00851f *
            // triangle_sequence[self->triangle.sequence];
        }

        { // dmc
            // timer clock
            if (--self->dmc.tval_counter <= 0) {
                self->dmc.tval_counter = self->dmc.tval;

                if (self->dmc.bits == 0 && (self->dmc.length_counter > 0)) {
                    self->dmc.buf =
                        bus_dmcdma(self->shared_bus, self->dmc.current_address);

                    // simulate cpu stall
                    delta_cycles += self->shared_counter->cycle_offset;
                    self->shared_counter->cycle_offset = 0;

                    if ((self->dmc.length_counter -= 1) == 0) {
                        if (self->reg.dmc.loop) {
                            // restart
                            self->dmc.length_counter = self->dmc.length;
                            self->dmc.current_address = self->dmc.address;
                        }
                        if (self->reg.dmc.int_enable) {
                            bus_assert_dmc_irq(self->shared_bus);
                        }
                    } else {
                        self->dmc.current_address =
                            (self->dmc.current_address == 0xFFFF)
                                ? 0x8000
                                : (self->dmc.current_address + 1);
                    }
                    self->dmc.bits = 8;
                }

                if (self->dmc.bits > 0) {
                    if (self->dmc.buf & 1) {
                        if (self->dmc.level <= 125) {
                            self->dmc.level += 2;
                        }
                    } else {
                        if (self->dmc.level >= 2) {
                            self->dmc.level -= 2;
                        }
                    }
                    self->dmc.buf >>= 1;
                    --self->dmc.bits;
                }
            }

            if (!self->channel_mute.dmc) {
                tnd_d += self->dmc.level;
            }
            // tnd_f += 0.00851f * dmc_sequence[self->dmc.sequence];
        }
        GUARD(tnd_d < 203);
        GUARD(pulse_d < 31);

        self->accum += self->tnd_table[tnd_d] + self->pulse_table[pulse_d];
        apu_event_emit_sample(self->shared_emitter, self, pulse_d, tnd_d);

        if (++self->accum_cnt == 30) {
            apu_event_emit_level30(self->shared_emitter, self, self->accum);
            self->accum = 0;
            self->accum_cnt = 0;
        }
    }

    return SUCCESS;
error:
    return NG;
}

int apu_write_reg(apu_t *self, uint16_t addr, uint8_t value)
{
    switch (addr) {
    case 0x4000:
        self->reg.$4000 = value;
        // TRACE("4000 %02x\n", value);
        break;
    case 0x4001:
        self->reg.$4001 = value;
        self->pulse1.sweep_divider = 0;
        self->pulse1.sweep_reload = true;
        self->pulse1.sweep_mute = false;
        // TRACE("4001 %02x\n", value);

        break;

    case 0x4002:
        self->reg.$4002 = value;
        self->pulse1.tval =
            (self->reg.pulse1.timer_low | (self->reg.pulse1.timer_high << 8)) +
            1;
        // TRACE("4002 %02x\n", value);
        break;
    case 0x4003:
        self->reg.$4003 = value;
        self->pulse1.tval =
            (self->reg.pulse1.timer_low | (self->reg.pulse1.timer_high << 8)) +
            1;
        self->pulse1.start = true;
        self->pulse1.sequence = 0;
        if (self->reg.status.pulse1) {
            self->pulse1.length_counter = length_lut[self->reg.pulse1.length];
        }
        break;
    case 0x4004:
        self->reg.$4004 = value;
        break;
    case 0x4005:
        self->reg.$4005 = value;
        self->pulse2.sweep_divider = 0;
        self->pulse2.sweep_reload = true;
        self->pulse2.sweep_mute = false;
        break;
    case 0x4006:
        self->reg.$4006 = value;
        self->pulse2.tval =
            (self->reg.pulse2.timer_low | (self->reg.pulse2.timer_high << 8)) +
            1;
        break;
    case 0x4007:
        self->reg.$4007 = value;
        self->pulse2.tval =
            (self->reg.pulse2.timer_low | (self->reg.pulse2.timer_high << 8)) +
            1;
        self->pulse2.start = true;
        self->pulse2.sequence = 0;
        if (self->reg.status.pulse2) {
            self->pulse2.length_counter = length_lut[self->reg.pulse2.length];
        }
        break;
    case 0x4008:
        self->reg.$4008 = value;
        self->triangle.linear = self->reg.triangle.linear;
        if (self->reg.triangle.control) {
            self->triangle.reload = true;
        }
        break;
    case 0x4009:
        self->reg.$4009 = value;
        break;
    case 0x400a:
        self->reg.$400a = value;
        self->triangle.tval = self->reg.triangle.timer_low |
                              (self->reg.triangle.timer_high << 8) + 1;
        break;
    case 0x400b:
        self->reg.$400b = value;
        self->triangle.tval = self->reg.triangle.timer_low |
                              (self->reg.triangle.timer_high << 8) + 1;
        // Note that writes to both $4008 and $400B will set the reload flag
        // and cause a reload of the linear counter at the next frame
        // sequence tick, even though $400B does not change the reload
        // value.
        GUARD(self->reg.triangle.length < 32);
        if (self->reg.status.triangle) {
            self->triangle.length_counter =
                length_lut[self->reg.triangle.length];
        }
        self->triangle.reload = true;
        break;
    case 0x400c:
        self->reg.$400c = value;
        // TRACE("%04x %02x\n", addr, value);
        break;
    case 0x400d:
        self->reg.$400d = value;
        // TRACE("%04x %02x\n", addr, value);
        break;
    case 0x400e:
        self->reg.$400e = value;
        self->noise.tval = noise_tval_lut[self->reg.noise.period];
        // TRACE("%04x %02x\n", addr, value);
        break;
    case 0x400f:
        self->reg.$400f = value;
        self->noise.start = true;
        if (self->reg.status.noise) {
            self->noise.length_counter = length_lut[self->reg.noise.length];
        }
        // TRACE("%04x %02x\n", addr, value);
        break;
    case 0x4010:
        self->reg.$4010 = value;
        self->dmc.tval = dmc_tval_lut[self->reg.dmc.rate];
        if (!self->reg.dmc.int_enable) {
            bus_clear_dmc_irq(self->shared_bus);
        }
        break;
    case 0x4011:
        self->reg.$4011 = value;
        // The DMC output level is set to D, an unsigned value. If the timer
        // is outputting a clock at the same time, the output level is
        // occasionally not changed properly
        self->dmc.level = self->reg.dmc.level;
        break;
    case 0x4012:
        self->reg.$4012 = value;
        self->dmc.current_address = self->dmc.address = 0xC000 | (value << 6);
        break;
    case 0x4013:
        self->reg.$4013 = value;
        self->dmc.length = (value << 4) + 1;
        if (self->reg.status.dmc) {
            self->dmc.length_counter = self->dmc.length;
        }
        break;
    case 0x4015:
        self->reg.$4015 = value;
        // When the enabled bit is cleared (via $4015), the length counter
        // is forced to 0 and cannot be changed until enabled is set again
        // (the length counter's previous value is lost). There is no
        // immediate effect when enabled is set.
        if (!self->reg.status.triangle) {
            self->triangle.length_counter = 0;
        }
        if (!self->reg.status.pulse1) {
            self->pulse1.length_counter = 0;
        }
        if (!self->reg.status.pulse2) {
            self->pulse2.length_counter = 0;
        }
        if (!self->reg.status.noise) {
            self->noise.length_counter = 0;
        }
        if (!self->reg.status.dmc) {
            self->dmc.length_counter = 0;
        } else {
            if (self->dmc.length_counter == 0) {
                // restart
                self->dmc.length_counter = self->dmc.length;
                self->dmc.current_address = self->dmc.address;
            }
        }
        bus_clear_dmc_irq(self->shared_bus);
        break;
    case 0x4017:
        self->reg.$4017 = value;
        if (self->reg.frame_counter.int_disable) {
            bus_clear_frame_irq(self->shared_bus);
            // TRACE("ACK\n");
        }
        // TODO: If the write occurs during an APU cycle, the effects occur
        // 3 CPU cycles after the $4017 write cycle, and if the write occurs
        // between APU cycles, the effects occurs 4 CPU cycles after the
        // write cycle.

        self->frame_phase = -1;
        if (self->cycles & 1) {
            self->next_qframe_cycles = 3;
        } else {
            self->next_qframe_cycles = 4;
        }
        self->cycles = 0;
        // DUMP8(self->reg.$4017);
        break;
    default:
        DIE("unexpected APU write: %04x %02x", addr, value);
        break;
    }

    return SUCCESS;
error:
    return NG;
}

uint8_t apu_peek_status(apu_t *self)
{
    uint8_t ret = (self->pulse1.length_counter > 0) |
                  ((self->pulse2.length_counter > 0) << 1) |
                  ((self->triangle.length_counter > 0) << 2) |
                  ((self->noise.length_counter > 0) << 3) |
                  ((self->dmc.length_counter > 0) << 4) |
                  ((self->shared_bus->ints.frame) << 6) |
                  ((self->shared_bus->ints.dmc) << 7);

    return ret;
}

uint8_t apu_read_status(apu_t *self)
{
    uint8_t ret = apu_peek_status(self);

    // The frame interrupt flag is connected to the CPU's IRQ line. It
    // is set at a particular point in the 4-step sequence (see below)
    // provided the interrupt inhibit flag in $4017 is clear, and can be
    // cleared either by reading $4015 (which also returns its old
    // status) or by setting the interrupt inhibit flag.
    bus_clear_frame_irq(self->shared_bus);

    return ret;
}

int apu_cold_boot(apu_t *self)
{
    GUARD(self->shared_bus);
    self->noise.shift = 0x0001;
    self->reg.$4015 = 0;

    return SUCCESS;
error:
    return NG;
}

void apu_destroy(apu_t *self)
{
    if (self) {
        FREE(self);
    }
}

apu_t *apu_create(apu_event_emitter_t *emitter)
{
    apu_t *self = NULL;

    TALLOC(self);
    GUARD(self->shared_emitter = emitter);

    for (int i = 0; i < 32; ++i) {
        self->pulse_table[i] = 95.52f / ((8128.0f / i) + 100);
    }

    for (int i = 0; i < 203; ++i) {
        self->tnd_table[i] = 163.67f / ((24329.0f / i) + 100);
    }

    self->next_qframe_cycles = frame_step_4[0];

    return self;
error:
    apu_destroy(self);
    return NULL;
}

int apu_set_bus(apu_t *self, bus_ref bus)
{
    GUARD(self);
    GUARD(!self->shared_bus);
    GUARD(self->shared_bus = bus);
    self->shared_counter = &self->shared_bus->counters.apu;

    return SUCCESS;
error:
    return NG;
}

void apu_soft_reset(apu_t *self)
{
    apu_write_reg(self, 0x4015, 0);
    self->triangle.sequence = 0;
    self->dmc.buf = 1;
    self->frame_phase = 0;
    self->next_qframe_cycles = frame_step_4[0];
}
