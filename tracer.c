#include "tracer.h"
#include <sys/time.h>

typedef struct hash_ctx {
    MD5_CTX hash;
    uint8_t *buf;
    size_t idx;
    size_t size;
    char hex[33];
} hash_ctx_t;

// TODO: displayの仕事を分離
typedef struct tracer {
    uint64_t checked_cycle;

    hash_ctx_t *video_hash;
    hash_ctx_t *audio_hash;
    hash_ctx_t *step_hash;

    // uint8_t step_buf[11 * 1024];
    // int step_idx;
    // MD5_CTX step_hash;

    struct timeval start_time;
} tracer_t;

void hash_ctx_destroy(hash_ctx_t *self)
{
    if (self) {
        FREE(self->buf)
        FREE(self);
    }
}

hash_ctx_t *hash_ctx_create(size_t size)
{
    hash_ctx_t *self = NULL;

    GUARD(size > 0);
    TALLOC(self);
    TALLOCS(self->buf, size);
    self->size = size;
    MD5_Init(&self->hash);

    return self;
error:
    hash_ctx_destroy(self);
    return NULL;
}

int hash_ctx_update(hash_ctx_t *self)
{
    if (self->idx > 0) {
        MD5_Update(&self->hash, self->buf, self->idx);
        self->idx = 0;
    }
    return SUCCESS;
}

int hash_ctx_update_on_full(hash_ctx_t *self)
{
    GUARD(self->idx <= self->size);
    if (self->idx == self->size) {
        hash_ctx_update(self);
    }
    return SUCCESS;
error:
    return NG;
}

int hash_ctx_flush(hash_ctx_t *self)
{
    MD5_CTX tmp;

    hash_ctx_update(self);
    memcpy(&tmp, &self->hash, sizeof(MD5_CTX));
    MD5_Final(&tmp);
    md5_hex(&tmp, self->hex);

    return SUCCESS;
}

void tracer_start(tracer_t *self)
{
    gettimeofday(&self->start_time, NULL);
}

void tracer_scanline_trace(tracer_t *self, int line, const uint16_t *dots)
{
    hash_ctx_t *hash = self->video_hash;

    memcpy(&hash->buf[hash->idx], dots, 512);
    hash->idx += 512;
    hash_ctx_update_on_full(hash);
}

void tracer_apu_trace(tracer_t *self, uint8_t pulse, uint8_t tnd)
{
    hash_ctx_t *hash = self->audio_hash;

    hash->buf[hash->idx++] = (pulse << 8) | tnd;
    hash_ctx_update_on_full(hash);
}

void tracer_cpu_trace(tracer_t *self, cpu_t *cpu)
{
    hash_ctx_t *hash = self->step_hash;
    uint8_t *buf = &hash->buf[hash->idx];
    scanline_t *scanline = &cpu->shared_bus->shared_ppu->scanline;

    *buf++ = cpu->reg.pc >> 8;
    *buf++ = cpu->reg.pc & 0xFF;
    *buf++ = cpu->reg.a;
    *buf++ = cpu->reg.x;
    *buf++ = cpu->reg.y;
    *buf++ = cpu->reg.p;
    *buf++ = cpu->reg.s;
    *buf++ = scanline->y >> 8;
    *buf++ = scanline->y & 0xFF;
    *buf++ = scanline->x >> 8;
    *buf++ = scanline->x & 0xFF;
    hash->idx += 11;

    hash_ctx_update_on_full(hash);
}

static inline void _flush_cpu(tracer_t *self)
{
    hash_ctx_flush(self->step_hash);
}

static inline void _flush_apu(tracer_t *self)
{
    hash_ctx_flush(self->audio_hash);
}

static inline void _flush_scanline(tracer_t *self)
{
    hash_ctx_flush(self->video_hash);
}

void tracer_verbose_flush_scanline(tracer_t *self, ppu_t *ppu)
{
    _flush_scanline(self);
    PRINT("%-16llu:%s\n", ppu->shared_counter->cycles, self->video_hash->hex);
}

void tracer_verbose_flush_apu(tracer_t *self, apu_t *apu)
{
    _flush_apu(self);
    PRINT("%-16llu:%s\n", apu->shared_counter->cycles, self->audio_hash->hex);
}

void tracer_verbose_flush_cpu(tracer_t *self, cpu_t *cpu)
{
    _flush_cpu(self);
    PRINT("%-16llu:%s\n", cpu->shared_counter->cycles, self->step_hash->hex);
}

int tracer_flush(tracer_t *self, core_t *core)
{
    uint64_t cpu_cycles = core->cpu->shared_counter->cycles;
    if (self->checked_cycle != cpu_cycles) {
        struct timeval current_time;

        gettimeofday(&current_time, NULL);

        double elapsed_ms =
            ((current_time.tv_sec - self->start_time.tv_sec) * 1000000.0 +
             (double)(current_time.tv_usec - self->start_time.tv_usec)) /
            1000.0;

        _flush_scanline(self);
        _flush_apu(self);
        _flush_cpu(self);

        PRINT("@Frame\t%lld\n", core->ppu->frames);
        PRINT("@Cycle\t%lld\n", cpu_cycles);
        PRINT("@Video\t%s\n", self->video_hash->hex);
        PRINT("@Audio\t%s\n", self->audio_hash->hex);
        PRINT("@Step\t%s\n", self->step_hash->hex);
        PRINT("@ROM\t%s\n", core->shared_ines->md5);
        PRINT("#Time\t%.3f\n", elapsed_ms);

        self->checked_cycle = cpu_cycles;
    }

    return SUCCESS;
}

void tracer_destroy(tracer_t *self)
{
    if (self) {
        hash_ctx_destroy(self->video_hash);
        hash_ctx_destroy(self->audio_hash);
        hash_ctx_destroy(self->step_hash);
        FREE(self);
    }
}

tracer_t *tracer_create(void)
{
    tracer_t *self = NULL;

    TALLOC(self);

    GUARD(self->video_hash = hash_ctx_create(256 * 2 * 240));
    GUARD(self->audio_hash = hash_ctx_create(128 * 1024));
    GUARD(self->step_hash = hash_ctx_create(11 * 16 * 1024));

    return self;
error:
    tracer_destroy(self);
    return NULL;
}
