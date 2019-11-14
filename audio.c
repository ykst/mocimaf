#include "audio.h"
#include <SDL.h>

#define MAXIMUM_APU_SAMPLING_RATE (1786830)
#define DEFAULT_SAMPLE_SIZE (1024)

typedef struct audio {
    apu_t *shared_apu;
    // inspector_t *shared_inspector;
    SDL_AudioSpec spec;

    bool paused;
    uint16_t *buf;
    int head;
    int tail;
    size_t buf_size;
    int sample_size;
} audio_t;

// uint8_t backup[32 * 1024] = {};

static inline int _buffered_samples(audio_t *self)
{
    int ret;

    if (self->head >= self->tail) {
        ret = self->head - self->tail;
    } else {
        ret = self->buf_size - self->tail + self->head;
    }

    return ret;
}

bool audio_needs_video_sync(audio_ref self)
{
    return !self->paused && (_buffered_samples(self) > self->sample_size * 2);
}

static void _sdl_audio_callback(void *context, uint8_t *stream, int len)
{
    audio_ref self = context;
    // apu_t *apu = self->shared_apu;

    // TRACE("%8d/%8d\n", apu->head, apu->tail);

    const size_t sample_size = sizeof(self->buf[0]);
    int samples = len / sample_size;
    void *start = &(self->buf[self->tail]);

    bool overrun = false;

    // DUMPD(apu->head);
    // DUMPD(apu->tail + samples);
    // DUMPD((apu->tail + samples) % apu->buf_size);
    // DUMPD(apu_buffered_samples(apu));
    if (_buffered_samples(self) < samples) {
        // self->shared_inspector->sound_overruns += 1;
        // memcpy(stream, backup, len);
        memset(stream, 0, len);
        return;
    }

    if (self->tail + samples >= self->buf_size) {
        overrun = ((self->tail + samples) % self->buf_size) > self->head;
    } else {
        overrun = self->tail + samples > self->head;
    }

    // if (overrun) {
    //    TRACE("OVERRUN\n");
    //    memset(stream, 0, len);
    //    return;
    //}

    int overrun_size = self->tail + samples - self->buf_size;
    if (overrun_size < 0) {
        memcpy(stream, start, len);
        // memcpy(backup, start, len);
        self->tail += samples;
    } else {
        int tail_size = (self->buf_size - self->tail) * sample_size;
        memcpy(stream, start, tail_size);
        // memcpy(backup, start, len);
        if (overrun_size != 0) {
            memcpy(&stream[tail_size], start, overrun_size * sample_size);
            // memcpy(&backup[tail_size], start, len);
        }
        self->tail = overrun_size;
    }
}

void audio_feed_level30(audio_ref self, float level30)
{
    self->buf[self->head++] =
        (int16_t)((int)((level30 * (float)0xFFFF) / 30.0) - 32768);
    if (self->head == self->buf_size) {
        self->head = 0;
    }
}

int audio_pause(audio_ref self, bool pause)
{
    self->paused = pause;
    self->tail = self->head = 0;

    SDL_PauseAudio(pause);

    return SUCCESS;
}

bool audio_toggle_pause(audio_ref self)
{
    audio_pause(self, !self->paused);
    return self->paused;
}

void audio_destroy(audio_ref self)
{
    if (self) {
        SDL_PauseAudio(1);
        SDL_CloseAudio();
        FREE(self->buf);
        FREE(self);
    }
}

audio_ref audio_create(apu_t *apu)
{
    audio_ref self = NULL;

    GUARD(SDL_InitSubSystem(SDL_INIT_AUDIO) == 0);
    TALLOC(self);

    self->shared_apu = apu;
    self->sample_size = DEFAULT_SAMPLE_SIZE;

    SDL_AudioSpec request = (SDL_AudioSpec){
        .freq = MAXIMUM_APU_SAMPLING_RATE / 30,
        .format = AUDIO_S16,
        .channels = 1,
        .samples = self->sample_size,
        .callback = _sdl_audio_callback,
        .userdata = self,
    };

    self->buf_size = (MAXIMUM_APU_SAMPLING_RATE / 30) * 4;
    TALLOCS(self->buf, self->buf_size);

    GUARD(SDL_OpenAudio(&request, &self->spec) == 0);

    audio_pause(self, true);

    return self;
error:
    audio_destroy(self);
    return NULL;
}
