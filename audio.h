#pragma once
#include "apu.h"
#include "utils.h"

typedef struct audio *audio_ref;

audio_ref audio_create(apu_t *apu);

int audio_pause(audio_ref self, bool pause);
bool audio_toggle_pause(audio_ref self);
void audio_destroy(audio_ref self);
void audio_feed_level30(audio_ref self, float level30);

bool audio_needs_video_sync(audio_ref self);
