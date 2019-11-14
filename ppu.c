#include "ppu.h"
#include "bus_internal.h"
#include "ines.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void _inc_scroll_coarce_x(ppu_scrolladdr_t *sa)
{
    if (sa->scroll.coarce_x == 0x1F) {
        sa->scroll.coarce_x = 0;
        sa->scroll.nametable_x ^= 1;
    } else {
        sa->scroll.coarce_x += 1;
    }
}

static inline uint32_t _bg_tile_fetch(ppu_t *self, ppu_scrolladdr_t sa)
{
    uint8_t *vram_page = bus_fetch_vram_page(
        self->shared_bus, sa.scroll.nametable_x, sa.scroll.nametable_y);

    uint16_t index = vram_page[sa.scroll.coarce_x + (sa.scroll.coarce_y << 5)];

    uint16_t tile_offset =
        (index << 4) + (self->reg.ctrl.background_addr ? 0x1000 : 0);

    uint8_t *tile = bus_fetch_chr(self->shared_bus, tile_offset);
    // uint16_t attr_base = page_offset[0x3C0];
    uint16_t attr_offset =
        (sa.scroll.coarce_x >> 2) + ((sa.scroll.coarce_y >> 2) << 3);
    uint8_t attr = vram_page[0x3C0 + attr_offset];
    uint8_t attr_slide =
        (sa.scroll.coarce_x & 0x2) | ((sa.scroll.coarce_y & 0x2) << 1);
    uint8_t palette_high = (attr >> attr_slide) & 0x3;
    uint8_t tile_low = tile[sa.scroll.fine_y];
    uint8_t tile_high = tile[sa.scroll.fine_y + 8];
    uint32_t pixels = self->tile_low_lut[tile_low] |
                      self->tile_high_lut[tile_high] |
                      self->tile_attr_lut[palette_high];

    return pixels;
}

void ppu_destroy(ppu_t *self)
{
    if (self) {
        FREE(self);
    }
}

static inline void ppu_transfer_scroll_y(ppu_t *self)
{
    self->scrolladdr_current.scroll.fine_y =
        self->scrolladdr_temporary.scroll.fine_y;

    self->scrolladdr_current.scroll.nametable_y =
        self->scrolladdr_temporary.scroll.nametable_y;

    self->scrolladdr_current.scroll.coarce_y =
        self->scrolladdr_temporary.scroll.coarce_y;
}

static inline void ppu_transfer_scroll_x(ppu_t *self)
{
    self->scrolladdr_current.scroll.nametable_x =
        self->scrolladdr_temporary.scroll.nametable_x;

    self->scrolladdr_current.scroll.coarce_x =
        self->scrolladdr_temporary.scroll.coarce_x;
}

static inline bool _rendering_is_enabled(ppu_t *self)
{
    return self->reg.mask.show_background || self->reg.mask.show_sprites;
}

static inline void _inc_current_scroll_y(ppu_t *self)
{
    uint16_t y = ((self->scrolladdr_current.scroll.coarce_y << 3) |
                  self->scrolladdr_current.scroll.fine_y) +
                 1;
    if (y == 240) {
        y = 0;
        self->scrolladdr_current.scroll.nametable_y =
            !self->scrolladdr_current.scroll.nametable_y;
    }
    self->scrolladdr_current.scroll.coarce_y = (y >> 3) & 0x1F;
    self->scrolladdr_current.scroll.fine_y = y & 0x7;
}

static void _state_enter_render_0_239(ppu_t *self, int *delta);
static void _state_render_0_239(ppu_t *self, int *delta);
static void _state_hblank(ppu_t *self, int *delta);
static void _state_empty_240(ppu_t *self, int *delta);
static void _state_enter_vblank_241(ppu_t *self, int *delta);
static void _state_vblank_241(ppu_t *self, int *delta);
static void _state_vblank_242_260(ppu_t *self, int *delta);
static void _state_enter_prerender_261(ppu_t *self, int *delta);
static void _state_prerender_261(ppu_t *self, int *delta);

static inline void _sprite_evaluation_atonce(ppu_t *self);
static inline void _sprite_fetch_atonce(ppu_t *self);
static inline void _draw_8dots(ppu_t *self);
static inline void _load_next_bg(ppu_t *self);

ppu_t *ppu_create(ppu_event_emitter_t *emitter)
{
    ppu_t *self = NULL;
    TALLOC(self);

    GUARD(self->shared_emitter = emitter);
    // self->shared_debugger = debugger;
    self->state_func = _state_enter_render_0_239;
    self->hblank_phase = HBLANK_START;

    for (uint16_t i = 0; i < 512; ++i) {
        uint8_t bg_idx = i & 0xF;
        uint8_t sp_idx = (i >> 4) & 0xF;
        uint8_t sp_attr = (i >> 8) & 0x1;
        bool bg_transparent = !(bg_idx & 0x3);
        bool sp_transparent = !(sp_idx & 0x3);
        uint8_t pallet_offset;

        if (bg_transparent && sp_transparent) {
            pallet_offset = 0;
        } else if (bg_transparent || !(sp_transparent || sp_attr)) {
            pallet_offset = sp_idx + 0x10;
        } else {
            pallet_offset = bg_idx;
        }

        self->mux_lut[sp_attr][i & 0xFF] = pallet_offset;
    }

    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t tile_low = 0;
        uint32_t tile_low_flip = 0;
        uint32_t tile_high = 0;
        uint32_t tile_high_flip = 0;

        for (int shift = 0; shift < 8; ++shift) {
            if ((i >> shift) & 0x1) {
                tile_low_flip |= 0x1 << (shift << 2);
                tile_high_flip |= 0x2 << (shift << 2);
                tile_low |= 0x1 << ((7 - shift) << 2);
                tile_high |= 0x2 << ((7 - shift) << 2);
            }
        }
        // TODO: L1キャッシュ効率的にはインタリーブした方が良い
        self->tile_low_lut[i] = tile_low;
        self->tile_low_flip_lut[i] = tile_low_flip;
        self->tile_high_lut[i] = tile_high;
        self->tile_high_flip_lut[i] = tile_high_flip;
    }

    for (uint32_t i = 0; i < 4; ++i) {
        uint32_t tile_attr = 0;

        for (int j = 0; j < 8; ++j) {
            tile_attr |= (i << 2) << (j << 2);
        }

        self->tile_attr_lut[i] = tile_attr;
    }

    return self;
error:
    ppu_destroy(self);
    return NULL;
}
static inline bool _consume(ppu_t *self, int *delta, int take)
{
    if (*delta >= take) {
        *delta -= take;
        return true;
    }
    return false;
}

static inline bool _should_skip_first_cycle(ppu_t *self)
{
    return (self->scanline.y == 0) && (self->frames & 1) &&
           self->reg.mask.show_background;
}

static inline void _new_scanline(ppu_t *self)
{
    self->scanline.x -= self->skipped_dot0 ? 340 : 341;

    if (self->scanline.x < 0)
        THROW("delta cycle underflow");

    self->scanline.y = (self->scanline.y + 1) % 262;
    self->skipped_dot0 = false;
}

static inline void _load_next_bg(ppu_t *self)
{
    if (self->reg.mask.show_background) {
        uint64_t next_tile = _bg_tile_fetch(self, self->scrolladdr_current);
        self->tile_pair = (self->tile_pair >> 32) | (next_tile << 32);
    }

    if (_rendering_is_enabled(self)) {
        _inc_scroll_coarce_x(&self->scrolladdr_current);
    }
}

static inline void _enter_renderline(ppu_t *self)
{
    // OAM2は外に露出しないので、この時点でクリアしても問題ない
    memset(&self->map.sprite_attrs2, 0xFF, sizeof(sprite_attribute_t) * 8);
    memcpy(self->sprite_dots, self->next_sprite_dots, 256);
    memset(self->next_sprite_dots, 0, 256);

    self->tile_cnt = 0;
}

static void _state_hblank(ppu_t *self, int *delta)
{
    switch (self->hblank_phase) {
    case HBLANK_START:
        // reload scroll x
        if (_consume(self, delta, 258 - 256)) {

            self->hblank_phase = HBLANK_SPRITE_FETCH;
        }
        break;
    case HBLANK_SPRITE_FETCH:
        // 本来ここは260-257相当なので、何かがおかしい
        if (_consume(self, delta, 261 - 258)) {
            if (_rendering_is_enabled(self)) {
                ppu_transfer_scroll_x(self);
            }
            self->hblank_phase = HBLANK_SPRITE_IDLE1;
        }
        break;
    case HBLANK_SPRITE_IDLE1:
        // idle
        if (_consume(self, delta, 279 - 261)) {
            _sprite_fetch_atonce(self);
            self->hblank_phase = HBLANK_SPRITE_IDLE2;
        }
        break;
    case HBLANK_SPRITE_IDLE2:
        // idle (increment scroll y on prerender line)
        if (self->scanline.y == 261 && _rendering_is_enabled(self)) {
            ppu_transfer_scroll_y(self);
        }
        if (_consume(self, delta, 304 - 279)) {
            self->hblank_phase = HBLANK_SPRITE_IDLE3;
        }
        break;
    case HBLANK_SPRITE_IDLE3:
        // idle
        if (_consume(self, delta, 320 - 304)) {
            self->hblank_phase = HBLANK_NEXT_BG1;
        }
        break;
    case HBLANK_NEXT_BG1:
        // bg prefetch 1
        if (_consume(self, delta, 328 - 320)) {
            _load_next_bg(self);
            self->hblank_phase = HBLANK_NEXT_BG2;
        }
        break;
    case HBLANK_NEXT_BG2:
        // bg prefetch 2
        if (_consume(self, delta, 336 - 328)) {
            _load_next_bg(self);
            self->hblank_phase = HBLANK_GARBAGE_FETCH;
        }
        break;
    case HBLANK_GARBAGE_FETCH:
        // garbage fetch (skips an idle cycle at line 0)
        if (_consume(self, delta, 4)) {
            _new_scanline(self);

            if (self->scanline.y == 240) {
                self->state_func = _state_empty_240;

            } else {
                self->state_func = _state_enter_render_0_239;
            }
            self->hblank_phase = HBLANK_START;
        }
        break;
    }
}

static void _state_render_0_239(ppu_t *self, int *delta)
{
    if (_consume(self, delta, 8)) {
        _draw_8dots(self);

        if (self->tile_cnt == 8) {
            _sprite_evaluation_atonce(self);
        }

        if (self->tile_cnt == 32) {
            ppu_event_emit_scanline(self->shared_emitter, self,
                                    self->scanline.y, self->scanline_dots);

            if (_rendering_is_enabled(self)) {
                _inc_current_scroll_y(self);
            }

            self->state_func = _state_hblank;
        }
    }
}

static void _state_enter_vblank_241(ppu_t *self, int *delta)
{
    if (_consume(self, delta, 1)) {
        // interrupts_t *ints = &self->shared_bus->ints;

        // if (!ints->cancell_vbl) {
        self->reg.status.vblank = 1;
        ppu_event_emit_vblank(self->shared_emitter, self);
        // if (self->shared_debugger) {
        //     debugger_log_append_unit(self->shared_debugger,
        //                              EVENT_LOG_VBL_ON);
        // }
        // PRINT("VBL %d %d\n", self->scanline.y, self->cycles);
        //} else {
        // PRINT("NO VBL %d %d\n", self->scanline.y, self->cycles);
        // ints->cancell_vbl = false;
        //}
        if (self->reg.ctrl.enable_nmi) {
            // if (!ints->cancell_nmi) {
            bus_assert_nmi(self->shared_bus);
            // PRINT("NMI %d %d\n", self->scanline.y,
            // self->cycles);
            //} else {
            // PRINT("NO NMI %d %d\n", self->scanline.y,
            // self->cycles);
            //}
        }

        self->frames += 1;

        ppu_event_emit_frame(self->shared_emitter, self, self->frames);

        self->state_func = _state_vblank_241;
    }
}

static void _state_enter_render_0_239(ppu_t *self, int *delta)
{
    if (_should_skip_first_cycle(self)) {
        self->skipped_dot0 = true;
        self->state_func = _state_render_0_239;
        _enter_renderline(self);
        // call next state immediately without modifying delta
        self->state_func(self, delta);
    } else if (_consume(self, delta, 1)) {
        _enter_renderline(self);
        self->state_func = _state_render_0_239;
    }
}

static void _state_vblank_241(ppu_t *self, int *delta)
{
    if (_consume(self, delta, 340)) {
        _new_scanline(self);
        self->state_func = _state_vblank_242_260;
    }
}

static void _state_empty_240(ppu_t *self, int *delta)
{
    if (_consume(self, delta, 341)) {

        _new_scanline(self);
        self->state_func = _state_enter_vblank_241;
    }
}

static inline void _draw_8dots(ppu_t *self)
{
    uint32_t pixels = self->tile_pair >> (self->scroll_fine_x << 2);

    uint8_t bg_transparent_mask =
        ((!self->reg.mask.show_background) ||
         (!self->reg.mask.show_left_bg && self->tile_cnt == 0))
            ? 0
            : 0xF;

    uint8_t sp_transparent_mask =
        ((!self->reg.mask.show_sprites) ||
         (!self->reg.mask.show_left_sprites && self->tile_cnt == 0))
            ? 0
            : 0xFF;

    uint16_t emphasis = self->reg.emphasis << 6;
    int x = self->tile_cnt * 8;

    for (int j = 0; j < 8; ++j) {
        uint8_t bg = pixels & bg_transparent_mask;
        uint8_t sp = self->sprite_dots[x] & sp_transparent_mask;

        pixels >>= 4;

        if (!self->reg.status.sprite0_hit && x < 255 && (sp & 0x10) &&
            ((bg & 0x3) && (sp & 0x3))) {
            self->reg.status.sprite0_hit = 1;
            ppu_event_emit_sprite0_hit(self->shared_emitter, self);
            // debugger_log_append_unit(self->shared_debugger,
            //                         EVENT_LOG_SPRITE0_ON);
        }

        bg &= self->hide_bg ? 0 : 0xFF;
        sp &= self->hide_sprites ? 0 : 0xFF;

        uint8_t mux_code = bg | (sp << 4);
        uint8_t priority = sp >> 5;
        uint8_t color_index =
            self->map.palettes[self->mux_lut[priority][mux_code]];

        self->scanline_dots[x] = emphasis | (color_index & 0x3F);

        ++x;
    }

    _load_next_bg(self);

    ++self->tile_cnt;
}

static inline void _sprite_evaluation_atonce(ppu_t *self)
{
    int oam_idx = 0;
    int oam2_idx = 0;

    bool sprite8x16 = self->reg.ctrl.sprite_size;
    int sprite_size = sprite8x16 ? 16 : 8;

    for (int i = 0; i < 8; ++i) {
        self->sprite_metas[i].oam_idx = -1;
    }

    while (oam_idx < 64) {
        sprite_attribute_t *attr = &self->map.sprite_attrs[oam_idx];
        int delta_y = self->scanline.y - attr->y;

        if (delta_y >= 0 && delta_y < sprite_size) {
            sprite_meta_t *meta = &self->sprite_metas[oam2_idx];

            self->map.sprite_attrs2[oam2_idx] = *attr;
            meta->oam_idx = oam_idx;

            if (attr->flip_y) {
                delta_y = sprite_size - delta_y - 1;
            }
            if (delta_y >= 8) {
                // offset of 8x16 upper bits
                delta_y += 8;
            }

            meta->delta_y = delta_y;

            if (++oam2_idx == 8) {
                break;
            }
        }
        ++oam_idx;
    }

    // sprite overflow bug
    int m = 0;

    while (oam_idx < 64) {
        int maybe_y = self->map.oam[oam_idx * 4 + m];

        if (self->scanline.y >= maybe_y &&
            self->scanline.y < maybe_y + sprite_size) {
            self->reg.status.sprite_overflow = 1;
            break;
        }

        m = (m + 1) & 0x3;
        ++oam_idx;
    }
}

static inline void _sprite_fetch_atonce(ppu_t *self)
{
    bool sprite8x16 = self->reg.ctrl.sprite_size;
    uint16_t sprite8x8_base = self->reg.ctrl.sprite_addr ? 0x1000 : 0;

    for (int i = 0; i < 8; ++i) {
        sprite_attribute_t *attr = &self->map.sprite_attrs2[i];

        sprite_meta_t *meta = &self->sprite_metas[i];
        uint16_t tile_offset =
            sprite8x16
                ? (((attr->index & 1) ? 0x1000 : 0) + ((attr->index & ~1) << 4))
                : (sprite8x8_base + (attr->index << 4));
        uint8_t *tile = bus_fetch_chr(self->shared_bus, tile_offset);

        if (attr->y == 0xFF) {
            break;
        }

        int delta_y = meta->delta_y;

        uint8_t tile_low = tile[delta_y];
        uint8_t tile_high = tile[delta_y + 8];
        uint8_t sprite0_mask = (meta->oam_idx == 0) ? 0x10 : 0;
        uint8_t priority_mask = attr->priority ? 0x20 : 0;
        uint32_t pixels = self->tile_attr_lut[attr->palette];

        if (attr->flip_x) {
            pixels |= self->tile_low_flip_lut[tile_low] |
                      self->tile_high_flip_lut[tile_high];
        } else {
            pixels |=
                self->tile_low_lut[tile_low] | self->tile_high_lut[tile_high];
        }

        int x = attr->x;
        for (int j = 0; j < 8 && x < 256; ++j) {
            if ((pixels & 0x3) && !(self->next_sprite_dots[x] & 0x3)) {
                self->next_sprite_dots[x] =
                    sprite0_mask | priority_mask | (pixels & 0xF);
            }
            pixels >>= 4;
            ++x;
        }
    }
}

static void _state_prerender_261(ppu_t *self, int *delta)
{
    if (_consume(self, delta, 8)) {
        _load_next_bg(self);

        if (++self->tile_cnt == 32) {
            self->state_func = _state_hblank;
        }
    }
}

static void _state_enter_prerender_261(ppu_t *self, int *delta)
{
    if (_consume(self, delta, 1)) {
        self->reg.status.vblank = 0;
        self->reg.status.sprite0_hit = 0;
        self->reg.status.sprite_overflow = 0;
        self->state_func = _state_prerender_261;
        _enter_renderline(self);
    }
}

static void _state_vblank_242_260(ppu_t *self, int *delta)
{
    if (_consume(self, delta, 341)) {
        _new_scanline(self);
        if (self->scanline.y == 261) {
            self->state_func = _state_enter_prerender_261;
        }
    }
}

// DRY
int ppu_set_bus(ppu_t *self, bus_ref bus)
{
    GUARD(self);
    GUARD(!self->shared_bus);
    GUARD(self->shared_bus = bus);
    self->shared_counter = &self->shared_bus->counters.ppu;

    return SUCCESS;
error:
    return NG;
}

int ppu_cold_boot(ppu_t *self)
{
    GUARD(self->shared_bus);

    return SUCCESS;
error:
    return NG;
}

void ppu_soft_reset(ppu_t *self)
{
    // NOP
}

int ppu_clock(ppu_t *self)
{
    if (self->shared_counter->cycle_offset == 0) {
        return SUCCESS;
    }

    int delta = self->dot_pos + self->shared_counter->cycle_offset;
    self->scanline.x += self->shared_counter->cycle_offset;

    self->shared_counter->cycle_offset = 0;

    int before_delta;

    do {
        before_delta = delta;
        self->state_func(self, &delta);
    } while (delta != before_delta);

    self->dot_pos = delta;

    return SUCCESS;
    // error:
    // return NG;
}

static inline uint8_t _read_status(ppu_t *self)
{
    uint8_t ret = self->reg.$2002;

    // Clear PPUADDR/PPUSCROLL write toggle
    self->scrolladdr_toggle = 0;

    // Clear vblank
    self->reg.$2002 = ret & 0x7F;
    // if (self->scanline.y == 241 &&
    //    self->cycles <= 1) { // && self->shared_ints->nmi) {
    //    // PRINT("CRITICAL %d@%d\n", self->shared_ints->nmi,
    //    self->cycles); if (self->cycles == 0) {
    //        self->shared_ints->cancell_nmi = true;
    //    } else {
    //        ret |= 0x80;
    //    }

    //    self->shared_ints->cancell_vbl = true;
    //}

    // Reading PPUSTATUS within two cycles of the start
    // of vertical blank will return 0 in bit 7 but clear the latch anyway,
    // causing NMI to not occur that frame.
    // if (ppu->cycles == 0) {
    //    if (ppu->scanline.y == 241) {
    //        ret &= 0x7F;
    //        self->int_nmi = false;
    //    } else if (ppu->scanline.y == 261) {
    //        // Same goes to pre-render line vice versa
    //        ret = ppu->shadow_status;
    //    }
    //}
    // NMIが立ち上がる寸前にVBLを読んだ場合の挙動
    // この瞬間のVBLは0が返ってくるが、ラッチがクリアされてNMIがキャンセルされる

    // int just_clock = 329; // 5LDA $2002 のみケア
    // if (self->scanline.y == 240) {
    //    if (self->cycles >= just_clock) {
    //        if (self->cycles == just_clock) {
    //            self->shared_ints->cancell_nmi = true;
    //        } else {
    //            ret |= 0x80;
    //        }
    //        self->shared_ints->cancell_vbl = true;
    //        // PRINT("EARLY %d\n", self->cycles);
    //    }
    //} else if (self->scanline.y == 260) {
    //    // 261ラインの1PPUサイクル時点で2002がクリアされるので、先取りする
    //    if (self->cycles > 326) {
    //        ret &= 0x1F;
    //        // PRINT("CLEAR %d\n", self->cycles);
    //    }
    //}

    // bits 0-5 is read from i/o bus
    ret = (ret & 0xE0) | (self->iobus_value & 0x1F);

    return ret;
}

static inline uint8_t _read_data(ppu_t *self)
{
    uint16_t addr = self->scrolladdr_current.value;
    uint8_t ret;
    if (addr >= 0x3f20 && addr < 0x4000) {
        addr = ((addr - 0x3f20) & 0x1F) + 0x3f00;
    }

    if (addr >= 0x3000 && addr <= 0x3EFF) {
        addr -= 0x1000;
    }

    if (addr >= 0x3F00 & addr < 0x4000) {
        // パレットは直接読み込めるが、リードバッファはVRAMのミラーアドレス相当の位置の値が読み込まれる
        // この場合はスプライトパレットの透明インデックスのミラーによるアドレスの修正は行われない
        self->ppudata_latch =
            *(bus_fetch_vram(self->shared_bus, addr - 0x1000));
        // self->map.name_tables[ppu_vram_offset(self, addr - 0x1000)];

        switch (addr) {
        case 0x3F10:
        case 0x3F14:
        case 0x3F18:
        case 0x3F1C:
            addr = addr - 0x10;
            break;
        }

        ret = self->map.palettes[addr & 0x1F];
    } else {
        ret = self->ppudata_latch;
        if (addr >= 0x2000 && addr < 0x3000) {
            self->ppudata_latch = *(bus_fetch_vram(self->shared_bus, addr));
            // self->map.name_tables[ppu_vram_offset(self, addr)];
        } else {
            self->ppudata_latch = *bus_fetch_chr(self->shared_bus, addr);
        }
    }

    self->scrolladdr_current.value += self->reg.ctrl.vram_inc ? 32 : 1;

    return ret;
}

static inline void _write_data(ppu_t *self, uint8_t value)
{
    uint16_t addr = self->scrolladdr_current.value & 0x3FFF;

    if (addr >= 0x3f20 && addr < 0x4000) {
        addr = ((addr - 0x3f20) & 0x1F) + 0x3f00;
    } else if (addr >= 0x3000 && addr <= 0x3EFF) {
        addr -= 0x1000;
    }

    switch (addr) {
    case 0x3F10:
    case 0x3F14:
    case 0x3F18:
    case 0x3F1C:
        addr = addr - 0x10;
        break;
    }

    if (addr >= 0x2000 && addr < 0x3000) {
        *(bus_fetch_vram(self->shared_bus, addr)) = value;
        // self->map.name_tables[ppu_vram_offset(self, addr)] = value;
    } else if (addr < 0x2000) {
        *bus_fetch_chr(self->shared_bus, addr) = value;
    } else {
        self->map.palettes[addr & 0x1F] = value;
    }

    if (addr < 0x2000) {
        // PRINT("illegal ppu write: %04x\n", addr);
    }

    self->scrolladdr_current.value += self->reg.ctrl.vram_inc ? 32 : 1;
}

static inline void _write_control(ppu_t *self, uint8_t value)
{
    if (!self->reg.ctrl.enable_nmi && (value & 0x80)) {
        if (self->reg.status.vblank) {
            self->shared_bus->ints.reserve_nmi = true;
        } else {
        }
    }

    self->reg.$2000 = value;
    self->iobus_value = value;
    self->scrolladdr_temporary.scroll.nametable_x =
        self->reg.ctrl.nametable_addr & 0x1;
    self->scrolladdr_temporary.scroll.nametable_y =
        (self->reg.ctrl.nametable_addr & 0x2) >> 1;
}

uint8_t ppu_read_reg(ppu_t *self, uint16_t addr)
{
    uint8_t ret;

    switch (addr) {
    case 0x2000:
        // WARN("Open bus: $2000\n");
        ret = self->iobus_value;
        break;
    case 0x2001:
        // WARN("Open bus: $2001\n");
        ret = self->iobus_value;
        break;
    case 0x2002:
        ret = _read_status(self);
        break;
    case 0x2003:
        // WARN("Open bus: $2003\n");
        ret = self->iobus_value;
        break;
    case 0x2004:
        ret = self->map.oam[self->reg.oamaddr];
        // The three unimplemented bits of each sprite's byte 2 do not exist
        // in the PPU and always read back as 0 on PPU revisions that allow
        // reading PPU OAM through OAMDATA ($2004).
        if ((self->reg.oamaddr & 0x3) == 2) {
            ret &= 0xE3;
        }
        self->iobus_value = ret;
        break;
    case 0x2005:
        // WARN("Open bus: $2005\n");
        ret = self->iobus_value;
        break;
    case 0x2006:
        // WARN("Open bus: $2006\n");
        ret = self->iobus_value;
        break;
    case 0x2007:
        ret = _read_data(self);
        // PPU memory read buffer is not the open bus
        self->iobus_value = ret; // self->ppudata_latch;
        // PRINT("r 2007@%04x %02x:%02x\n", self->ppuaddr_latch, ret,
        // self->ppudata_latch);
        break;
    default:
        THROW("unexpected PPU read: %04x", addr);
        break;
    }

    ppu_event_emit_reg(self->shared_emitter, self, true, ret, addr);

    return ret;
}

static inline void _addr_write(ppu_t *self, uint8_t value)
{
    // debugger_log_append_unit(self->shared_debugger, EVENT_LOG_PPUADDR_WRITE);

    if (self->scrolladdr_toggle == 0) {
        // self->scrolladdr_temporary.value =
        //    (self->scrolladdr_temporary.value & 0x00FF) | (value << 8);
        self->scrolladdr_temporary.addr.pch = value & 0x3F;
    } else {
        self->scrolladdr_temporary.addr.pcl = value;
        // self->scrolladdr_temporary.value =
        //    (self->scrolladdr_temporary.value & 0xFF00) | value;
        self->scrolladdr_current.value = self->scrolladdr_temporary.value;
    }

    // ppu_debug_dump_scrolls(self, "AW");

    self->scrolladdr_toggle = !self->scrolladdr_toggle;
}

static inline void _scroll_write(ppu_t *self, uint8_t value)
{
    // debugger_log_append_unit(self->shared_debugger, EVENT_LOG_PPUSCRL_WRITE);

    if (self->scrolladdr_toggle == 0) {
        self->scroll_fine_x = value & 0x7;
        self->scrolladdr_temporary.scroll.coarce_x = value >> 3;
    } else {
        self->scrolladdr_temporary.scroll.coarce_y = value >> 3;
        self->scrolladdr_temporary.scroll.fine_y = value & 0x7;
    }

    // ppu_debug_dump_scrolls(self, "SW");

    self->scrolladdr_toggle = !self->scrolladdr_toggle;
}

void ppu_write_reg(ppu_t *self, uint16_t addr, uint8_t value)
{
    ppu_event_emit_reg(self->shared_emitter, self, false, value, addr);

    switch (addr) {
    case 0x2000:
        _write_control(self, value);
        break;
    case 0x2001:
        // if (self->cycles >= 29658) {
        self->reg.$2001 = value;
        // }
        self->iobus_value = value;
        // self->shared_ints->enable_rendering =
        // _rendering_is_enabled(self);
        break;
    case 0x2002:
        // WARN("Open bus write: $2002\n");
        self->iobus_value = value;
        break;
    case 0x2003:
        // TODO: タイミングによりglitchが発生する
        self->reg.oamaddr = value;
        self->iobus_value = value;
        break;
    case 0x2004:
        self->map.oam[self->reg.oamaddr] = value;
        self->reg.oamaddr = (self->reg.oamaddr + 1) & 0xFF;
        self->iobus_value = value;
        break;
    case 0x2005:
        _scroll_write(self, value);
        self->iobus_value = value;
        break;
    case 0x2006:
        _addr_write(self, value);
        self->iobus_value = value;
        break;
    case 0x2007:
        _write_data(self, value);
        self->iobus_value = value;
        break;
    default:
        THROW("unexpected PPU write: %04x <- %02x", addr, value);
        break;
    }
}
