#pragma once
#include "utils.h"

typedef union joypad_reg {
    struct {
        uint8_t a : 1;
        uint8_t b : 1;
        uint8_t select : 1;
        uint8_t start : 1;
        uint8_t up : 1;
        uint8_t down : 1;
        uint8_t left : 1;
        uint8_t right : 1;
    };
    uint8_t buttons;
} joypad_reg_t;

typedef struct joypad {
    bool strobe;
    int shift;
    joypad_reg_t reg;
} joypad_t;

static inline uint8_t joypad_read(joypad_t *self)
{
    uint8_t ret = (self->reg.buttons >> self->shift) & 1;
    self->shift = (self->shift + 1) % 8;
    return ret;
}

static inline void joypad_write(joypad_t *self, uint8_t value)
{
    self->strobe = value & 0x01;
    if (self->strobe) {
        self->shift = 0;
    }
}
