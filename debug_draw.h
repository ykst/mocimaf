#pragma once
#include "ppu.h"
#include "utils.h"

int debug_draw_grid(ppu_t *ppu, uint32_t *lut, uint32_t *argb_256x240);
int debug_draw_nametable(ppu_t *ppu, uint32_t *lut, uint32_t *argb_512x480);
int debug_draw_pattern(ppu_t *ppu, uint32_t *lut, uint32_t *argb_256x128);
int debug_draw_palette(ppu_t *ppu, uint32_t *lut, uint32_t *argb_4x9);
