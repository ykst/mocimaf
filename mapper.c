#include "mapper.h"
#include "bus_internal.h"

static inline uint8_t **_split_rom_pages(uint8_t *mem, uint32_t size,
                                         uint16_t split, int *out_pages)
{
    int pages = size / split;

    uint8_t **mems = NULL;
    TALLOCS(mems, pages);

    for (int i = 0; i < pages; ++i) {
        mems[i] = &mem[split * i];
    }

    *out_pages = pages;

    return mems;
error:
    return NULL;
}

static inline uint8_t **_split_chr_pages(mapper_t *self, const ines_t *ines,
                                         uint16_t split, int *out_pages)
{
    self->chr_split = split;
    return _split_rom_pages(ines->chr, 0x2000 * ines->header->chr_page_counts,
                            split, out_pages);
}

static inline uint8_t **_split_prg_pages(mapper_t *self, const ines_t *ines,
                                         uint16_t split, int *out_pages)
{
    self->prg_split = split;
    return _split_rom_pages(ines->prg, 0x4000 * ines->header->prg_page_counts,
                            split, out_pages);
}

// NROM
typedef struct mapper0 {
    mapper_t base;
    uint8_t *prgs[2];
} mapper0_t;

// MMC1
typedef struct mapper1 {
    mapper_t base;
    uint8_t shift;
    int shift_count;

    // 0: one-screen, lower bank; 1: one-screen, upper bank;
    // 2: vertical; 3: horizontal
    union {
        struct {
            uint8_t mirror_control : 2;
            uint8_t rom_mode : 2;
            uint8_t chr_mode : 1;
            uint8_t : 3;
        };
        uint8_t control;
    };
    uint8_t chr0_idx;
    uint8_t chr1_idx;
    uint8_t prg_idx;
    uint8_t *chrs[2];
    uint8_t *prgs[2];
} mapper1_t;

// UxROM
typedef struct mapper2 {
    mapper_t base;
    int prg_idx;
    uint8_t *prgs[2];
} mapper2_t;

// CNROM
typedef struct mapper3 {
    mapper_t base;
    int chrom_idx;
    uint8_t *prgs[2];
} mapper3_t;

// MMC3/MMC6
typedef struct mapper4 {
    mapper_t base;

    union {
        struct {
            uint8_t bank_select : 3;
            uint8_t : 2;
            uint8_t prg_ram_enable : 1; // MMC6
            uint8_t prg_rom_bank_mode : 1;
            uint8_t chr_rom_bank_mode : 1;
        };
        uint8_t $8000;
    };

    union {
        struct {
            // Nametable mirroring (0: vertical; 1: horizontal)
            // This bit has no effect on cartridges with hardwired 4-screen
            // VRAM.
            uint8_t mirroring : 1;
            uint8_t : 7;
        };
        uint8_t $a000;
    };

    union {
        // MMC3
        struct {
            uint8_t : 6;
            uint8_t write_protection : 1;
            uint8_t prg_ram_chip_enable : 1;
        };
        // MMC6
        struct {
            uint8_t : 4;
            uint8_t enable_ram_write_low : 1;
            uint8_t enable_ram_read_low : 1;
            uint8_t enable_ram_write_high : 1;
            uint8_t enable_ram_read_high : 1;
        };
        uint8_t $a001;
    };

    uint8_t irq_latch;
    uint8_t irq_count;
    bool irq_reload;
    bool irq_enabled;

    uint8_t *prgs[4];
    uint8_t *chrs[8];

    uint16_t prev_page;
} mapper4_t;

int nop_on_rom_write(mapper_t *base, uint16_t addr, uint8_t value)
{
    // NOP
    return SUCCESS;
}

uint8_t *trivial_on_chr_read(mapper_t *base, uint16_t addr)
{
    return &base->chr_pages[0][addr];
}

uint8_t mapper0_on_prg_read(mapper_t *base, uint16_t addr)
{
    mapper0_t *self = container_of(mapper0_t, base, base);
    uint8_t ret = self->prgs[addr >= 0xC000][addr & 0x3FFF];

    return ret;
}

uint32_t mapper0_on_prg_phys(const mapper_t *base, uint16_t addr)
{
    mapper0_t *self = container_of(mapper0_t, base, base);
    uint8_t *p = &self->prgs[addr >= 0xC000][addr & 0x3FFF];
    return (uintptr_t)p - (uintptr_t)base->prg_base;
}

vram_mirror_mode_t ines_mirror_mode(const ines_t *ines)
{
    vram_mirror_mode_t mode;

    if (ines->header->four_screen_mirroring) {
        mode = VRAM_MIRROR_FOURSCREEN;
    } else if (ines->header->mirroring) {
        mode = VRAM_MIRROR_VERTICAL;
    } else {
        mode = VRAM_MIRROR_HORIZONTAL;
    }

    return mode;
}

bool mapper0_on_boundary_check(const mapper_t *base, uint32_t phys1,
                               uint32_t phys2)
{
    return true;
}

mapper_t *mapper0_create(const ines_t *ines)
{
    mapper0_t *self = NULL;
    TALLOC(self);
    self->base.mirror_mode = ines_mirror_mode(ines);
    self->base.bus_conflict = false;
    self->base.on_rom_write = nop_on_rom_write;
    self->base.on_chr_read = trivial_on_chr_read;
    self->base.on_prg_read = mapper0_on_prg_read;
    self->base.on_prg_phys = mapper0_on_prg_phys;
    self->base.on_boundary_check = mapper0_on_boundary_check;

    GUARD(self->base.chr_pages = _split_chr_pages(&self->base, ines, 0x2000,
                                                  &self->base.num_chr_pages));
    GUARD(self->base.prg_pages = _split_prg_pages(&self->base, ines, 0x4000,
                                                  &self->base.num_prg_pages));

    self->prgs[0] = self->base.prg_pages[0];
    if (self->base.num_prg_pages == 1) {
        self->prgs[1] = self->base.prg_pages[0];
    } else {
        self->prgs[1] = self->base.prg_pages[1];
    }
    return &self->base;
error:
    FREE(self);
    return NULL;
}

int mapper1_on_rom_write(mapper_t *base, uint16_t addr, uint8_t value)
{
    mapper1_t *self = container_of(mapper1_t, base, base);

    if (value & 0x80) {
        self->shift_count = 0;
        self->shift = 0x10;
        self->control = 0x0C;
        base->mirror_mode = VRAM_MIRROR_ONE_LOWER;
        // printf("*** RC %04x %02x\n", addr, value);
    } else {
        self->shift = ((value & 1) << 4) | (self->shift >> 1);
        // printf("RS %04x %02x %02x\n", addr, value, self->shift);
        if (++self->shift_count == 5) {
            int reg = ((addr & 0x6000) >> 13) & 0x3;
            // printf("*** R:%d C:%d\n", reg, self->shift);

            switch (reg) {
            case 0: // $8000
                self->control = self->shift;
                switch (self->mirror_control) {
                case 0:
                    base->mirror_mode = VRAM_MIRROR_ONE_LOWER;
                    break;
                case 1:
                    base->mirror_mode = VRAM_MIRROR_ONE_UPPER;
                    break;
                case 2:
                    base->mirror_mode = VRAM_MIRROR_VERTICAL;
                    break;
                case 3:
                    base->mirror_mode = VRAM_MIRROR_HORIZONTAL;
                    break;
                default:
                    DIE("unexpected mirror mode: %d\n", self->mirror_control);
                    break;
                }
                break;
            case 1: // $A000
                self->chr0_idx = self->shift;
                // self->chr0_idx %= base->ines->header->chr_page_counts * 2;
                GUARD(self->chr0_idx < base->num_chr_pages);
                if (self->chr_mode == 1) {
                    // 4K mode
                    self->chrs[0] = base->chr_pages[self->chr0_idx];
                } else {
                    // 8K mode
                    uint8_t idx = self->chr0_idx;
                    self->chrs[0] = base->chr_pages[idx & ~1];
                    self->chrs[1] = base->chr_pages[idx | 1];
                }

                break;
            case 2: // $C0000
                self->chr1_idx = self->shift;
                if (self->chr_mode == 1) {
                    // only 4K mode is supported
                    GUARD(self->chr1_idx < base->num_chr_pages);
                    self->chrs[1] = base->chr_pages[self->chr1_idx];
                }
                break;
            case 3: // $E000
                self->prg_idx =
                    self->shift & 0x0F; // bit 4 = PRG RAM chip enable
                GUARD(self->prg_idx < base->num_prg_pages);
                switch (self->rom_mode) {
                case 0:
                case 1:
                    // 0, 1: switch 32 KB at $8000, ignoring low bit of bank
                    // number;
                    self->prgs[0] = base->prg_pages[(self->prg_idx & ~0x1)];
                    self->prgs[1] = base->prg_pages[(self->prg_idx | 0x1)];
                    break;
                case 2:
                    // 2: fix first bank at $8000 and switch 16 KB bank at
                    // $C000;
                    self->prgs[1] = base->prg_pages[self->prg_idx];
                    break;
                case 3:
                    // 3: fix last bank at $C000 and switch 16 KB bank at $8000)
                    self->prgs[0] = base->prg_pages[self->prg_idx];
                    self->prgs[1] = base->prg_pages[base->num_prg_pages - 1];
                    break;
                default:
                    DIE("unexpected rom mode: %d\n", self->rom_mode);
                    break;
                }

                break;
            default:
                DIE("unexpected MMC1 register: %d\n", reg);
                break;
            }

            self->shift_count = 0;
            self->shift = 0x10;
        }
    }

    return SUCCESS;
error:
    return NG;
}

uint8_t *mapper1_on_chr_read(mapper_t *base, uint16_t addr)
{
    mapper1_t *self = container_of(mapper1_t, base, base);
    return &self->chrs[addr >= 0x1000][addr & 0x0FFF];
}

uint8_t mapper1_on_prg_read(mapper_t *base, uint16_t addr)
{
    mapper1_t *self = container_of(mapper1_t, base, base);
    return self->prgs[addr >= 0xC000][addr & 0x3FFF];
}

uint32_t mapper1_on_prg_phys(const mapper_t *base, uint16_t addr)
{
    mapper1_t *self = container_of(mapper1_t, base, base);
    uint8_t *p = &self->prgs[addr >= 0xC000][addr & 0x3FFF];
    return (uintptr_t)p - (uintptr_t)base->prg_base;
}

bool mapper1_on_boundary_check(const mapper_t *base, uint32_t addr1,
                               uint32_t addr2)
{
    return (addr1 & ~(base->prg_split - 1)) == (addr2 & ~(base->prg_split - 1));
}

mapper_t *mapper1_create(const ines_t *ines)
{
    mapper1_t *self = NULL;

    TALLOC(self);
    self->base.bus_conflict = false;
    self->base.mirror_mode = VRAM_MIRROR_ONE_LOWER;
    self->base.on_rom_write = mapper1_on_rom_write;
    self->base.on_chr_read = mapper1_on_chr_read;
    self->base.on_prg_read = mapper1_on_prg_read;
    self->base.on_prg_phys = mapper1_on_prg_phys;
    self->base.on_boundary_check = mapper1_on_boundary_check;

    GUARD(ines->header->prg_page_counts >= 2);

    GUARD(self->base.chr_pages = _split_chr_pages(&self->base, ines, 0x1000,
                                                  &self->base.num_chr_pages));
    GUARD(self->base.prg_pages = _split_prg_pages(&self->base, ines, 0x4000,
                                                  &self->base.num_prg_pages));

    self->chrs[0] = self->base.chr_pages[0];
    self->chrs[1] = self->base.chr_pages[1];

    self->prgs[0] = self->base.prg_pages[0];
    self->prgs[1] = self->base.prg_pages[self->base.num_prg_pages - 1];

    self->control = 0x0C;

    return &self->base;
error:
    FREE(self);
    return NULL;
}

int mapper3_on_rom_write(mapper_t *base, uint16_t addr, uint8_t value)
{
    int idx = value & 0x3;
    mapper3_t *self = container_of(mapper3_t, base, base);

    GUARD(idx < base->num_chr_pages);
    self->chrom_idx = idx;

    return SUCCESS;
error:
    return NG;
}

uint8_t *mapper3_on_chr_read(mapper_t *base, uint16_t addr)
{
    mapper3_t *self = container_of(mapper3_t, base, base);

    return &base->chr_pages[self->chrom_idx][addr];
}

uint8_t mapper3_on_prg_read(mapper_t *base, uint16_t addr)
{
    mapper3_t *self = container_of(mapper3_t, base, base);
    return self->prgs[addr >= 0xC000][addr & 0x3FFF];
}

uint32_t mapper3_on_prg_phys(const mapper_t *base, uint16_t addr)
{
    mapper3_t *self = container_of(mapper3_t, base, base);
    uint8_t *p = &self->prgs[addr >= 0xC000][addr & 0x3FFF];
    return (uintptr_t)p - (uintptr_t)base->prg_base;
}

bool mapper3_on_boundary_check(const mapper_t *base, uint32_t addr1,
                               uint32_t addr2)
{
    return true;
}

mapper_t *mapper3_create(const ines_t *ines)
{
    mapper3_t *self = NULL;

    TALLOC(self);
    self->base.bus_conflict = true;
    self->base.mirror_mode = ines_mirror_mode(ines);
    self->base.on_rom_write = mapper3_on_rom_write;
    self->base.on_prg_read = mapper3_on_prg_read;
    self->base.on_prg_phys = mapper3_on_prg_phys;
    self->base.on_chr_read = mapper3_on_chr_read;
    self->base.on_boundary_check = mapper3_on_boundary_check;
    self->chrom_idx = 0;

    GUARD(self->base.chr_pages = _split_chr_pages(&self->base, ines, 0x2000,
                                                  &self->base.num_chr_pages));

    GUARD(self->base.prg_pages = _split_prg_pages(&self->base, ines, 0x4000,
                                                  &self->base.num_prg_pages));

    self->prgs[0] = self->base.prg_pages[0];
    if (self->base.num_prg_pages == 1) {
        self->prgs[1] = self->base.prg_pages[0];
    } else {
        self->prgs[1] = self->base.prg_pages[1];
    }

    return &self->base;
error:
    FREE(self);
    return NULL;
}

int mapper2_on_rom_write(mapper_t *base, uint16_t addr, uint8_t value)
{
    int idx = value & 0xf;
    mapper2_t *self = container_of(mapper2_t, base, base);

    GUARD(idx < base->num_prg_pages);
    self->prg_idx = idx;
    self->prgs[0] = base->prg_pages[self->prg_idx];

    return SUCCESS;
error:
    return NG;
}

uint8_t mapper2_on_prg_read(mapper_t *base, uint16_t addr)
{
    mapper2_t *self = container_of(mapper2_t, base, base);
    return self->prgs[addr >= 0xC000][addr & 0x3FFF];
}

uint32_t mapper2_on_prg_phys(const mapper_t *base, uint16_t addr)
{
    mapper2_t *self = container_of(mapper2_t, base, base);
    uint8_t *p = &self->prgs[addr >= 0xC000][addr & 0x3FFF];
    return (uintptr_t)p - (uintptr_t)base->prg_base;
}

bool mapper2_on_boundary_check(const mapper_t *base, uint32_t addr1,
                               uint32_t addr2)
{
    return (addr1 & ~(base->prg_split - 1)) == (addr2 & ~(base->prg_split - 1));
}

mapper_t *mapper2_create(const ines_t *ines)
{
    mapper2_t *self = NULL;

    TALLOC(self);
    self->base.bus_conflict = true;
    self->base.mirror_mode = ines_mirror_mode(ines);
    self->base.on_rom_write = mapper2_on_rom_write;
    self->base.on_prg_read = mapper2_on_prg_read;
    self->base.on_prg_phys = mapper2_on_prg_phys;
    self->base.on_chr_read = trivial_on_chr_read;
    self->base.on_boundary_check = mapper2_on_boundary_check;
    self->prg_idx = 0;

    GUARD(self->base.prg_pages = _split_prg_pages(&self->base, ines, 0x4000,
                                                  &self->base.num_prg_pages));

    GUARD(self->base.chr_pages = _split_chr_pages(&self->base, ines, 0x2000,
                                                  &self->base.num_chr_pages));

    self->prgs[0] = self->base.prg_pages[0];
    self->prgs[1] = self->base.prg_pages[self->base.num_prg_pages - 1];

    return &self->base;
error:
    FREE(self);
    return NULL;
}

uint8_t *mapper4_on_chr_read(mapper_t *base, uint16_t addr)
{
    mapper4_t *self = container_of(mapper4_t, base, base);
    int page = addr >> 10;
    if (page >= 4 && self->prev_page < 4) {
        if (self->irq_count == 0 || self->irq_reload) {
            self->irq_count = self->irq_latch;
            self->irq_reload = false;
        } else {
            --self->irq_count;
        }

        if (self->irq_count == 0 && self->irq_enabled) {
            bus_assert_ext_irq(base->shared_bus);
        }
    }
    self->prev_page = page;
    return &self->chrs[page][addr & 0x03FF];
}

int mapper4_on_rom_write(mapper_t *base, uint16_t addr, uint8_t value)
{
    mapper4_t *self = container_of(mapper4_t, base, base);
    int reg = ((addr & 0x6000) >> 13) & 0x3;
    bool is_odd = addr & 1;

    // printf("MMC3: %04x %02x\n", addr, value);

    switch (reg) {
    case 0: // $8000
        if (is_odd) {
            if (self->bank_select <= 5) {
                // GUARD(value < base->num_chr_pages);
                value %= base->num_chr_pages;
                switch (self->bank_select) {
                case 0: {
                    int target = self->chr_rom_bank_mode ? 4 : 0;

                    self->chrs[target] = base->chr_pages[value];
                    self->chrs[target + 1] =
                        base->chr_pages[(value + 1) % base->num_chr_pages];
                    break;
                }
                case 1: {
                    int target = self->chr_rom_bank_mode ? 6 : 2;

                    self->chrs[target] = base->chr_pages[value];
                    self->chrs[target + 1] =
                        base->chr_pages[(value + 1) % base->num_chr_pages];
                    break;
                }
                case 2:
                case 3:
                case 4:
                case 5: {
                    int target = self->chr_rom_bank_mode
                                     ? (self->bank_select - 2)
                                     : (self->bank_select + 2);
                    self->chrs[target] = base->chr_pages[value];
                    break;
                }
                }
            } else {
                // GUARD(value < base->num_prg_pages);
                value %= base->num_prg_pages;
                switch (self->bank_select) {
                case 6:
                    if (self->prg_rom_bank_mode) {
                        self->prgs[0] =
                            base->prg_pages[base->num_prg_pages - 2];
                        self->prgs[2] = base->prg_pages[value];
                    } else {
                        self->prgs[0] = base->prg_pages[value];
                        self->prgs[2] =
                            base->prg_pages[base->num_prg_pages - 2];
                    }
                    break;
                case 7:
                    self->prgs[1] = base->prg_pages[value];
                    break;
                }
            }
        } else {
            self->$8000 = value;
        }
        break;
    case 1: // $A000
        if (is_odd) {
            // ignore protections for MMC6 compat
            self->$a001 = value;
        } else {
            self->$a000 = value;
            base->mirror_mode =
                self->mirroring ? VRAM_MIRROR_HORIZONTAL : VRAM_MIRROR_VERTICAL;
        }
        break;
    case 2: // $C000
        // printf("IRQ1: %04x %02x\n", addr, value);
        if (is_odd) {
            self->irq_reload = true;
            self->irq_count = 0;
        } else {
            self->irq_latch = value;
        }
        break;
    case 3: // $E000
        // printf("IRQ2: %04x %02x\n", addr, value);
        if (is_odd) {
            self->irq_enabled = true;
        } else {
            self->irq_enabled = false;
            bus_clear_ext_irq(base->shared_bus);
        }
        break;
    }
    return SUCCESS;
    // error:
    // return NG;
}

uint8_t mapper4_on_prg_read(mapper_t *base, uint16_t addr)
{
    mapper4_t *self = container_of(mapper4_t, base, base);
    return self->prgs[(addr >> 13) & 0x3][addr & 0x1FFF];
}

uint32_t mapper4_on_prg_phys(const mapper_t *base, uint16_t addr)
{
    mapper4_t *self = container_of(mapper4_t, base, base);
    uint8_t *p = &self->prgs[(addr >> 13) & 0x3][addr & 0x1FFF];
    return (uintptr_t)p - (uintptr_t)base->prg_base;
}

bool mapper4_on_boundary_check(const mapper_t *base, uint32_t addr1,
                               uint32_t addr2)
{
    return (addr1 & ~(base->prg_split - 1)) == (addr2 & ~(base->prg_split - 1));
}

mapper_t *mapper4_create(const ines_t *ines)
{
    mapper4_t *self = NULL;

    TALLOC(self);
    self->base.bus_conflict = false;
    self->base.mirror_mode = ines_mirror_mode(ines);
    self->base.on_rom_write = mapper4_on_rom_write;
    self->base.on_prg_read = mapper4_on_prg_read;
    self->base.on_prg_phys = mapper4_on_prg_phys;
    self->base.on_chr_read = mapper4_on_chr_read;
    self->base.on_boundary_check = mapper4_on_boundary_check;
    self->prgs[0] = ines->prg;
    self->prgs[1] = ines->prg;
    self->prgs[2] = ines->prg;
    self->prgs[3] =
        &ines->prg[(ines->header->prg_page_counts - 1) * 0x4000 + 0x2000];

    GUARD(self->base.chr_pages = _split_chr_pages(&self->base, ines, 0x400,
                                                  &self->base.num_chr_pages));
    GUARD(self->base.prg_pages = _split_prg_pages(&self->base, ines, 0x2000,
                                                  &self->base.num_prg_pages));

    // CHR initial state unknown
    for (int i = 0; i < 8; ++i) {
        self->chrs[i] = self->base.chr_pages[self->base.num_chr_pages - 1];
    }

    return &self->base;
error:
    // TODO: destroy
    FREE(self);
    return NULL;
}

mapper_t *mapper_create(const ines_t *ines)
{
    mapper_t *mapper = NULL;

    switch (ines->mapper) {
    case 0:
        GUARD(mapper = mapper0_create(ines));
        break;
    case 1:
        GUARD(mapper = mapper1_create(ines));
        break;
    case 2:
        GUARD(mapper = mapper2_create(ines));
        break;
    case 3:
        GUARD(mapper = mapper3_create(ines));
        break;
    case 4:
        GUARD(mapper = mapper4_create(ines));
        break;
    default:
        WARN("unsupported mapper %d\n", ines->mapper);
        GUARD(mapper = mapper0_create(ines));
        break;
    }

    mapper->id = ines->mapper;
    mapper->chr_base = ines->chr;
    mapper->prg_base = ines->prg;
    mapper->prg_size = ines->prg_size;
    mapper->chr_size = ines->chr_size;

    return mapper;
error:
    return NULL;
}

int mapper_set_bus(mapper_t *self, bus_ref bus)
{
    self->shared_bus = bus;
    return SUCCESS;
}
