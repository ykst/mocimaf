#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

typedef struct ines_header {
    uint8_t magic[4];
    uint8_t prg_page_counts;
    // Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
    uint8_t chr_page_counts;

    struct {
        uint8_t mirroring : 1; // 0/1 = Horizontal/Vertical
        uint8_t sram_enabled : 1;
        // The Trainer Area follows the 16-byte Header and precedes the PRG-ROM
        // area if bit 2 of Header byte 6 is set. It is always 512 bytes in size
        // if present, and contains data to be loaded into CPU memory at $7000.
        // It is only used by some games that were modified to run on different
        // hardware from the original cartridges, such as early RAM cartridges
        // and emulators, and which put some additional compatibility code into
        // those address ranges.
        uint8_t trainer_present : 1;
        // 1: Ignore mirroring control or above mirroring bit; instead provide
        // four-screen VRAM
        uint8_t four_screen_mirroring : 1;
        uint8_t mapper_low : 4;
    };
    struct {
        uint8_t console_type : 2;
        uint8_t version_magic : 2;
        uint8_t mapper_high : 4;
    };
    struct {
        uint8_t mapper_ext : 4;
        uint8_t submapper : 4;
    };
    struct {
        uint8_t prg_rom_size : 4;
        uint8_t chr_rom_size : 4;
    };
    struct {
        uint8_t prg_ram_shift_count : 4;
        uint8_t prg_sram_shift_count : 4;
    };
    struct {
        uint8_t chr_ram_shift_count : 4;
        uint8_t chr_nvram_shift_count : 4;
    };
    struct {
        uint8_t timing_mode : 2;
        uint8_t : 6;
    };
    struct {
        uint8_t vs_ppu_type : 4;
        uint8_t vs_hw_type : 4;
    };
    struct {
        uint8_t misc_roms : 2;
        uint8_t : 6;
    };
    struct {
        uint8_t expansion_device : 6;
        uint8_t : 2;
    };
} ines_header_t;

typedef struct ines {
    int version;
    off_t size;
    char *name;
    int mapper;
    ines_header_t *header;
    bool chr_ram;
    uint8_t *buf;
    uint8_t *trainer;
    // contiguous PRG
    uint8_t *prg;
    // contiguous CHR
    uint8_t *chr;
    size_t prg_size;
    size_t chr_size;
    char md5[33];
} ines_t;

ines_t *ines_from_file(const char *path);
ines_t *ines_from_buffer(const char *name, uint8_t *buf, off_t size);
void ines_dump_info(ines_t *self);
