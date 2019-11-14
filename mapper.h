#pragma once
#include "bus.h"
#include "ines.h"
#include "inspector.h"
#include "interrupts.h"
#include "utils.h"

typedef enum vram_mirror_mode {
    VRAM_MIRROR_VERTICAL = 0,
    VRAM_MIRROR_HORIZONTAL,
    VRAM_MIRROR_FOURSCREEN,
    VRAM_MIRROR_ONE_LOWER,
    VRAM_MIRROR_ONE_UPPER,
} vram_mirror_mode_t;

typedef struct mapper {
    int id;
    vram_mirror_mode_t mirror_mode;
    bool bus_conflict;
    int (*on_rom_write)(struct mapper *self, uint16_t addr, uint8_t value);
    uint8_t *(*on_chr_read)(struct mapper *self, uint16_t addr);
    uint8_t (*on_prg_read)(struct mapper *self, uint16_t addr);
    uint32_t (*on_prg_phys)(const struct mapper *self, uint16_t addr);
    uint8_t (*on_misc_read)(struct mapper *self, uint16_t addr);
    int (*on_misc_write)(struct mapper *self, uint16_t addr, uint8_t value);
    bool (*on_boundary_check)(const struct mapper *self, uint32_t addr1,
                              uint32_t to);

    // interrupts_t *shared_ints;
    bus_ref shared_bus;
    // void (*destroy)(struct mapper *self);
    uint8_t **chr_pages;
    uint8_t **prg_pages;
    int num_chr_pages;
    int num_prg_pages;
    uint8_t *chr_base;
    uint8_t *prg_base;
    uint16_t prg_split;
    uint16_t chr_split;
    uint32_t prg_size;
    uint32_t chr_size;

} mapper_t;

mapper_t *mapper_create(const ines_t *ines);
int mapper_set_bus(mapper_t *self, bus_ref bus);
