// *** WARNING ***
// This file is generated by genevents.rb
// Keep untouched or ruin the abstraction
#pragma once
#include "utils.h"
#include "ppu.h"

typedef void (*ppu_event_callback_on_scanline_t)(void *context, ppu_ref ppu, int line, const uint16_t * dots);
typedef void (*ppu_event_callback_on_frame_t)(void *context, ppu_ref ppu, uint64_t frames);
typedef void (*ppu_event_callback_on_sprite0_hit_t)(void *context, ppu_ref ppu);
typedef void (*ppu_event_callback_on_vblank_t)(void *context, ppu_ref ppu);
typedef void (*ppu_event_callback_on_reg_t)(void *context, ppu_ref ppu, bool is_read, uint8_t value, ppu_reg_addr_t target);

typedef struct ppu_event_listener {
    void *context;
    // provides packed pixels of a processed scanline, called on right after entering hblank
    ppu_event_callback_on_scanline_t on_scanline;
    // called on entering vblank, regardless of rendering was enabled or not
    ppu_event_callback_on_frame_t on_frame;
    // called on sprite 0 hit
    ppu_event_callback_on_sprite0_hit_t on_sprite0_hit;
    // called on set vblank flag
    ppu_event_callback_on_vblank_t on_vblank;
    // called on register read/write
    ppu_event_callback_on_reg_t on_reg;
} ppu_event_listener_t;