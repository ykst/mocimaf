#include "ines.h"
#include "md5.h"
#include "utils.h"
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

uint8_t *read_file(const char *path, off_t *o_size)
{
    struct stat stat = {};
    int fd = -1;
    uint8_t *buf = NULL;

    GUARD((fd = open(path, O_RDONLY)) != -1);
    GUARD(fstat(fd, &stat) == 0);
    GUARD(stat.st_mode & S_IFREG);
    GUARD(!(stat.st_mode & S_IFDIR));
    GUARD(buf = malloc(stat.st_size));

    off_t remains = stat.st_size;
    off_t cnt = 0;
    while (remains > 0) {
        GUARD(cnt = read(fd, &buf[stat.st_size - remains], remains));
        remains -= cnt;
    }

    CLOSE(fd);

    *o_size = stat.st_size;

    return buf;
error:
    CLOSE(fd);
    FREE(buf);
    return NULL;
}

int ines_validate(ines_t *ines)
{
    ines_header_t *header = ines->header;
    uint8_t *magic = header->magic;

    GUARD(*magic++ == 'N' && *magic++ == 'E' && *magic++ == 'S' &&
          *magic == 0x1A);

    off_t expected_size =
        sizeof(ines_header_t) + header->prg_page_counts * 0x4000 +
        header->chr_page_counts * 0x2000 + (header->trainer_present ? 512 : 0);

    GUARD(expected_size == ines->size);

    ines->version = (ines->header->version_magic == 0x08) ? 2 : 1;

    return SUCCESS;
error:
    return NG;
}

void ines_destroy(ines_t *ines)
{
    if (ines != NULL) {
        if (ines->chr_ram) {
            FREE(ines->chr);
        }
        FREE(ines->name);
        FREE(ines);
        // TODO: バッファは所有していないので、誰かが消さないとリークする
    }
}

ines_t *ines_from_buffer(const char *name, uint8_t *buf, off_t size)
{
    ines_t *self = NULL;

    TALLOC(self);
    self->buf = buf;
    self->size = size;
    self->name = name ? strndup(name, 255) : "NES";

    self->header = (ines_header_t *)self->buf;
    self->mapper = self->header->mapper_low | (self->header->mapper_high << 4),

    GUARD(ines_validate(self));

    off_t offset = sizeof(ines_header_t);
    if (self->header->trainer_present) {
        self->trainer = &self->buf[offset];
        offset += 512;
    }

    // TALLOCS(self->prgs, self->header->prg_page_counts);
    self->prg = &self->buf[offset];
    offset += 0x4000 * self->header->prg_page_counts;

    if (self->header->chr_page_counts > 0) {
        self->chr = &self->buf[offset];
        offset += 0x2000 * self->header->chr_page_counts;
    } else {
        // CHR-RAM
        GUARD(self->chr = calloc(0x2000, 1));
        self->header->chr_page_counts = 1;
        self->chr_ram = true;
    }

    self->prg_size = self->header->prg_page_counts * 0x4000;
    self->chr_size = self->header->chr_page_counts * 0x2000;

    MD5_CTX hash;
    MD5_Init(&hash);
    MD5_Update(&hash, buf, self->size);
    MD5_Final(&hash);
    md5_hex(&hash, self->md5);

    return self;
error:
    ines_destroy(self);
    return NULL;
}

ines_t *ines_from_file(const char *path)
{
    uint8_t *buf = NULL;
    off_t size = 0;
    GUARD(buf = read_file(path, &size));

    return ines_from_buffer(path, buf, size);
error:
    FREE(buf);
    return NULL;
}

void ines_dump_info(ines_t *self)
{
    PRINT("VER:%d MAPPER:%d CHR:%d PRG:%d MIRROR:%d "
          "FSCREEN:%d SRAM:%d "
          "CHRRAM:%d\n",
          self->version, self->mapper, self->header->chr_page_counts,
          self->header->prg_page_counts, self->header->mirroring,
          self->header->four_screen_mirroring, self->header->sram_enabled,
          self->chr_ram);

    if (self->version == 2) {
        PRINT("+++ iNES v2 +++\n"
              "MAPPER_EXT:%d SUB_MAPPER:%d PRG_ROM_SIZE:%d "
              "CHR_ROM_SIZE:%d PRG_RAM_SHIFT:%d "
              "PRG_SRAM_SHIFT:%d "
              "CHR_RAM_SHIFT:%d CHR_NVRAM_SHIFT:%d TIMING:%d\n",
              self->header->mapper_ext, self->header->submapper,
              self->header->prg_rom_size, self->header->chr_rom_size,
              self->header->prg_ram_shift_count,
              self->header->prg_sram_shift_count,
              self->header->chr_ram_shift_count,
              self->header->chr_nvram_shift_count, self->header->timing_mode);
    }
}
