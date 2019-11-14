#include "debug_draw.h"
#include "apu.h"
#include "inspector.h"
#include "ppu.h"

int debug_draw_grid(ppu_t *ppu, uint32_t *lut, uint32_t *argb)
{
    // uint32_t bg1 = palette_lut_argb[cpu->ppu->map.palette.image[0]];
    // uint32_t bg2 = bg1
    for (int y = 0; y < 240; ++y) {
        for (int x = 0; x < 256; ++x) {
            if (x % 32 == 0 || y % 32 == 0) {
                *argb = 0xFFFF8080;
            } else if (x % 16 == 0 || y % 16 == 0) {
                *argb = 0xFFFFFFFF;
            } else if (x % 8 == 0 || y % 8 == 0) {
                *argb = 0xFF808080;
            }
            ++argb;
        }
    }
    return SUCCESS;
}

int debug_draw_palette(ppu_t *ppu, uint32_t *lut, uint32_t *argb)
{
    uint8_t *palette = ppu->map.palette.image;

    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            argb[y * 4 + x] = lut[palette[y * 4 + x]];
        }
    }
    palette = ppu->map.palette.sprite;
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            argb[(y + 5) * 4 + x] = lut[palette[y * 4 + x]];
        }
    }
    return SUCCESS;
}

int debug_draw_pattern(ppu_t *ppu, uint32_t *lut, uint32_t *argb)
{
#if 0
    uint8_t *palette = ppu->map.palette.image;

    for (int page = 0; page < 2; ++page) {
        int offset = 0;
        for (int ty = 0; ty < 16; ++ty) {
            for (int tx = 0; tx < 16; ++tx) {
                uint8_t *tile = ppu->shared_mapper->on_chr_read(
                    ppu->shared_mapper, page * 0x1000 + offset);
                for (int y = 0; y < 8; ++y) {
                    uint8_t bits0 = tile[y];
                    uint8_t bits1 = tile[y + 8];
                    for (int x = 7; x >= 0; --x) {
                        int idx = (bits0 & 1) | ((bits1 & 1) << 1);

                        uint32_t color = lut[palette[idx]];
                        argb[(ty * 8 + y) * 256 + (page * 128) + tx * 8 + x] =
                            color;

                        bits0 >>= 1;
                        bits1 >>= 1;
                    }
                }
                offset += 16;
            }
        }
    }
#endif
    return SUCCESS;
}

// XXX: temporary workaround
// static bool show_attributes = false;

int debug_draw_nametable(ppu_t *ppu, uint32_t *lut, uint32_t *argb)
{
#if 0
    uint8_t *palette = ppu->map.palette.image;

    for (int page = 0; page < 4; ++page) {
        // uint8_t *table = &ppu->map.tables[page];
        int py = page / 2 * 240;
        int px = (page % 2) * 256;
        for (int ty = 0; ty < 30; ++ty) {
            for (int tx = 0; tx < 32; ++tx) {
                uint16_t vram_offset =
                    ppu_fetch_nt_vram_offset(ppu, px + tx * 8, py + ty * 8);
                uint16_t tile_offset = ppu_fetch_tile_offset(ppu, vram_offset);
                uint16_t attr_base = (vram_offset & ~1023) + 0x3C0;
                uint8_t *tile = ppu->shared_mapper->on_chr_read(
                    ppu->shared_mapper, tile_offset);

                for (int y = 0; y < 8; ++y) {
                    uint8_t bits0 = tile[y];
                    uint8_t bits1 = tile[y + 8];
                    int bg_y = py + (ty * 8 + y);
                    for (int x = 7; x >= 0; --x) {
                        int bg_x = px + (tx * 8 + x);
                        int idx = (bits0 & 1) | ((bits1 & 1) << 1);

                        uint8_t ax = bg_x & 0xFF;
                        uint8_t ay = bg_y % 240;
                        uint16_t attr_offset =
                            attr_base + (ax >> 5) + ((ay >> 5) << 3);
                        uint8_t attr = ppu->map.vram[attr_offset];
                        uint8_t palette_high =
                            (attr >>
                             (((!!((ax & 0x10)) | (!!(ay & 0x10) << 1)) * 2))) &
                            0x3;
                        uint32_t color;
                        if (show_attributes) {
                            color = lut[palette[(palette_high << 2) |
                                                ((x > 3) | ((y > 3) << 1))]];
                        } else {
                            color = lut[palette[(palette_high << 2) | idx]];
                        }
                        argb[(ty * 8 + y + 240 * (page / 2)) * 512 +
                             ((page % 2) * 256) + tx * 8 + x] = color;

                        bits0 >>= 1;
                        bits1 >>= 1;
                    }
                }
            }
        }
    }

    { // highlight scroll window
        for (int y = 0; y < 240; ++y) {
            int sx = ppu->raster_x[y];
            int sy = ppu->raster_y[y];
            argb[(sy % 480) * 512 + (sx % 512)] = 0xFF00FF00;
            argb[(sy % 480) * 512 + (sx + 256) % 512] = 0xFF00FF00;
        }

        for (int x = 0; x < 256; ++x) {
            // int sx = ppu->raster_x[y];
            // int sy = ppu->raster_y[y];
            argb[ppu->raster_y[0] * 512 + (ppu->raster_x[0] + x) % 512] =
                0xFF00FF00;
            argb[(ppu->raster_y[239] % 480) * 512 +
                 (ppu->raster_x[239] + x) % 512] = 0xFF00FF00;
        }
    }

    { // highlight sprite 0
        int sx = ppu->map.sprite_attrs[0].x;
        int sy = ppu->map.sprite_attrs[0].y;
        int h = ppu->reg.ctrl.sprite_size ? 15 : 7;
        for (int y = 0; y < h; ++y) {
            argb[((sy + y) % 480) * 512 + sx] = 0xFFFF0000;
            argb[((sy + y) % 480) * 512 + (sx + 7) % 512] = 0xFFFF0000;
        }
        for (int x = 0; x < 8; ++x) {
            argb[sy * 512 + (sx + x) % 512] = 0xFFFF0000;
            argb[((sy + h) % 480) * 512 + (sx + x) % 512] = 0xFFFF0000;
        }
    }
#endif
    return SUCCESS;
}
