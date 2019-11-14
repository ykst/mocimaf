#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bus.h"
#include "counter.h"
#include "ines.h"

typedef struct sprite_attribute {
    // Sprite data is delayed by one scanline; you must subtract 1 from the
    // sprite's Y coordinate before writing it here. Hide a sprite by writing
    // any values in $EF-$FF here. Sprites are never displayed on the first line
    // of the picture, and it is impossible to place a sprite partially off the
    // top of the screen.
    uint8_t y;

    union {
        struct {
            // Bank ($0000 or $1000) of tiles
            uint8_t bank : 1;
            // Tile number of top of sprite (0 to 254; bottom half gets the next
            // tile)
            uint8_t num : 7;
        } mode8x16;
        uint8_t index;
    };
    union {
        struct {
            uint8_t palette : 2;
            uint8_t : 3; // always 0
            // (0: in front of background; 1: behind background)
            uint8_t priority : 1;
            // Flipping does not change the position of the sprite's bounding
            // box, just the position of pixels within the sprite.
            uint8_t flip_x : 1;
            // In 8x16 mode, vertical flip flips each of the subtiles and also
            // exchanges their position; the odd-numbered tile of a vertically
            // flipped sprite is drawn on top.
            uint8_t flip_y : 1;
        };
        uint8_t attr;
    };

    // X-scroll values of $F9-FF results in parts of the sprite to be past the
    // right edge of the screen, thus invisible. It is not possible to have a
    // sprite partially visible on the left edge. Instead, left-clipping through
    // PPUMASK ($2001) can be used to simulate this effect.
    uint8_t x;

} __attribute__((packed)) sprite_attribute_t;

// refs: http://wiki.nesdev.com/w/index.php/PPU_scrolling
typedef union ppu_scrolladdr {
    struct {
        uint16_t pcl : 8;
        uint16_t pch : 8; // upper 2bit are always cleared on write
    } addr;
    struct {
        uint16_t coarce_x : 5;
        uint16_t coarce_y : 5;
        uint16_t nametable_x : 1;
        uint16_t nametable_y : 1;
        uint16_t fine_y : 3;
        uint16_t : 1;
    } scroll;
    uint16_t value;
} ppu_scrolladdr_t;

typedef struct sprite_meta {
    int delta_y;
    int oam_idx;
} sprite_meta_t;

typedef enum hblank_state {
    HBLANK_START = 0,
    HBLANK_SPRITE_FETCH,
    HBLANK_SPRITE_IDLE1,
    HBLANK_SPRITE_IDLE2,
    HBLANK_SPRITE_IDLE3,
    HBLANK_NEXT_BG1,
    HBLANK_NEXT_BG2,
    HBLANK_GARBAGE_FETCH,
} hblank_state_t;

typedef struct ppu_map {
    union {
        struct {
            uint8_t image[0x10];
            uint8_t sprite[0x10];
        } palette;
        uint8_t palettes[0x20];
    };

    union {
        sprite_attribute_t sprite_attrs[64];
        uint8_t oam[0x100];
    };

    union {
        sprite_attribute_t sprite_attrs2[8];
        uint8_t oam2[32];
    };
} ppu_map_t;

typedef enum ppu_reg_addr {
    PPU_REG_ADDR_PPUCTRL = 0x2000,
    PPU_REG_ADDR_PPUMASK = 0x2001,
    PPU_REG_ADDR_PPUSTAT = 0x2002,
    PPU_REG_ADDR_OAMADDR = 0x2003,
    PPU_REG_ADDR_OAMDATA = 0x2004,
    PPU_REG_ADDR_PPUSCRL = 0x2005,
    PPU_REG_ADDR_PPUADDR = 0x2006,
    PPU_REG_ADDR_PPUDATA = 0x2007,
} ppu_reg_addr_t;

static inline const char *ppu_reg_addr_show(ppu_reg_addr_t reg)
{
    switch (reg) {
    case PPU_REG_ADDR_PPUCTRL:
        return "PPUCTRL";
        break;
    case PPU_REG_ADDR_PPUMASK:
        return "PPUMASK";
        break;
    case PPU_REG_ADDR_PPUSTAT:
        return "PPUSTAT";
        break;
    case PPU_REG_ADDR_OAMADDR:
        return "OAMADDR";
        break;
    case PPU_REG_ADDR_OAMDATA:
        return "OAMDATA";
        break;
    case PPU_REG_ADDR_PPUSCRL:
        return "PPUSCRL";
        break;
    case PPU_REG_ADDR_PPUADDR:
        return "PPUADDR";
        break;
    case PPU_REG_ADDR_PPUDATA:
        return "PPUDATA";
        break;
    default:
        return NULL;
    }
}

typedef struct ppu_reg {
    // http://wiki.nesdev.com/w/index.php/PPU_registers
    union {
        struct {
            // (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
            uint8_t nametable_addr : 2;
            // (0: add 1, going across; 1: add 32, going down)
            uint8_t vram_inc : 1;
            // (0: $0000; 1: $1000; ignored in 8x16 mode)
            uint8_t sprite_addr : 1;
            // (0: $0000; 1: $1000)
            uint8_t background_addr : 1;
            // (0: 8x8 pixels; 1: 8x16 pixels)
            uint8_t sprite_size : 1;

            // (0: read backdrop from EXT pins; 1: output color on EXT pins)
            // The stock NES grounds these pin.
            uint8_t master_slave : 1;

            // (0 : off; 1 : on)
            // If the PPU is currently in vertical blank, and the PPUSTATUS
            // ($2002) vblank flag is still set (1), changing the NMI flag
            // in bit 7 of $2000 from 0 to 1 will immediately generate an
            // NMI. This can result in graphical errors (most likely a
            // misplaced scroll) if the NMI routine is executed too late in
            // the blanking period to finish on time. To avoid this problem
            // it is prudent to read $2002 immediately before writing $2000
            // to clear the vblank flag.
            uint8_t enable_nmi : 1;
        } ctrl;
        // After power/reset, writes to this register are ignored for about
        // 30,000 cycles.
        uint8_t $2000;
    };
    union {
        struct {
            // (0: normal color, 1: produce a greyscale display)
            // causes the palette to use only the colors from the grey
            // column: $00, $10, $20, $30. This is implemented as a bitwise
            // AND with $30 on any value read from PPU $3F00-$3FFF
            uint8_t grayscale : 1;
            // 1: Show background in leftmost 8 pixels of screen, 0: Hide
            uint8_t show_left_bg : 1;
            // 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
            uint8_t show_left_sprites : 1;
            // 1: Show background
            uint8_t show_background : 1;
            // 1: Show sprites
            uint8_t show_sprites : 1;
            // Each bit emphasizes 1 color while darkening the other two.
            // Setting all three emphasis bits will darken colors $00-$0D,
            // $10-$1D, $20-$2D, and $30-$3D (see PPU palettes). Note that
            // $1D black is affected by color emphasis, but $0F black is
            // not.
            uint8_t emphasize_red : 1;   // PAL: green
            uint8_t emphasize_green : 1; // PAL: red
            uint8_t emphasize_blue : 1;  // PAL: blue
        } mask;
        struct {
            uint8_t : 5;
            uint8_t emphasis : 3;
        };
        uint8_t $2001;
    };
    union {
        struct {
            uint8_t mirror : 5;
            uint8_t sprite_overflow : 1;
            uint8_t sprite0_hit : 1;
            uint8_t vblank : 1;
        } status;

        uint8_t $2002;
    };
    union {
        uint8_t oamaddr;
        uint8_t $2004;
    };
} ppu_reg_t;

typedef struct ppu *ppu_ref;
#include "ppu_event_emitter.h"

typedef struct scanline {
    uint16_t y;
    uint16_t x;
} scanline_t;

typedef struct ppu {
    ppu_map_t map;
    ppu_reg_t reg;

    bus_ref shared_bus;

    uint8_t iobus_value;
    uint8_t ppudata_latch;

    ppu_scrolladdr_t scrolladdr_temporary;
    ppu_scrolladdr_t scrolladdr_current;
    uint8_t scroll_fine_x;
    bool scrolladdr_toggle;

    // ppu_counter_t counter;
    bool has_sprite0;
    // uint16_t raster_x[240];
    // uint16_t raster_y[240];
    // ppu_scrolladdr_t scrolls[240];

    uint64_t frames;
    scanline_t scanline;

    counter_t *shared_counter;
    ppu_event_emitter_t *shared_emitter;
    // debugger_ref shared_debugger;

    uint16_t current_line[256];

    uint8_t mux_lut[2][256];
    uint32_t tile_low_lut[256];
    uint32_t tile_high_lut[256];
    uint32_t tile_low_flip_lut[256];
    uint32_t tile_high_flip_lut[256];
    uint32_t tile_attr_lut[4];
    uint8_t sprite_dots[256];
    uint8_t next_sprite_dots[256];
    uint16_t scanline_dots[256];
    uint64_t sprite_cache[8];
    sprite_meta_t sprite_metas[8];

    bool hide_bg;
    bool hide_sprites;

    // void *on_scanline_break_ctx;
    // void (*on_scanline_break)(void *ctx, struct ppu *self);

    int tile_cnt;
    uint64_t tile_pair;
    hblank_state_t hblank_phase;
    bool skipped_dot0;
    int dot_pos;

    void (*state_func)(struct ppu *self, int *delta);
} ppu_t;

static inline void ppu_scrolladdr_get_scrolls(ppu_t *self,
                                              ppu_scrolladdr_t *reg, int *out_x,
                                              int *out_y)
{
    *out_x = ((reg->scroll.coarce_x << 3) | (self->scroll_fine_x)) +
             (reg->scroll.nametable_x ? 256 : 0);
    *out_y = ((reg->scroll.coarce_y << 3) | (reg->scroll.fine_y)) +
             (reg->scroll.nametable_y ? 240 : 0);
}

uint16_t ppu_fetch_nt_vram_offset(ppu_t *self, int x, int y);
uint16_t ppu_fetch_tile_offset(ppu_t *self, uint16_t vram_offset);

void ppu_destroy(ppu_t *self);
ppu_t *ppu_create(ppu_event_emitter_t *emitter);
void ppu_init_mem(ppu_t *self, ines_t *ines);

uint8_t ppu_read_reg(ppu_t *self, uint16_t addr);
void ppu_write_reg(ppu_t *self, uint16_t addr, uint8_t value);
int ppu_set_bus(ppu_t *self, bus_ref bus);
// int ppu_simulate_cycles(ppu_t *self, uint32_t delta_cpu_cycles);
int ppu_cold_boot(ppu_t *self);
void ppu_soft_reset(ppu_t *self);
int ppu_clock(ppu_t *self);

uint16_t ppu_vram_offset(ppu_t *self, uint16_t addr);
