#pragma once
#include "joypad.h"
#include "utils.h"

typedef enum playlog_flag {
    PLAYLOG_ENTRY_SYSTEM = 0x1,
    PLAYLOG_ENTRY_JOYPAD1 = 0x2,
    PLAYLOG_ENTRY_JOYPAD2 = 0x4,
    PLAYLOG_ENTRY_REPEAT16 = 0xFE,
    PLAYLOG_ENTRY_REPEAT32 = 0xFF,
} playlog_flag_t;

typedef enum playlog_system { PLAYLOG_SYSTEM_RESET = 0x1 } playlog_system_t;

typedef struct recorder *recorder_ref;
void recorder_destroy(recorder_ref self);
recorder_ref recorder_create(const char *path);
int recorder_append(recorder_ref self, uint8_t system, joypad_reg_t joypad1,
                    joypad_reg_t joypad2);
const char *recorder_get_path(recorder_ref self);
int recorder_append_reset(recorder_ref self);

typedef struct playback *playback_ref;
void playback_destroy(playback_ref self);
playback_ref playback_create(const char *path);
int playback_step(playback_ref self, uint8_t *system, joypad_reg_t *joypad1,
                  joypad_reg_t *joypad2);
bool playback_finished(playback_ref self);
