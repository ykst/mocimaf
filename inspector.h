#pragma once
#include "interrupts.h"
#include "utils.h"

typedef struct inspector {

    bool quit;

    union {
        struct {
            uint8_t pause_frame : 1;
            uint8_t pause_scanline : 1;
            uint8_t pause_instruction : 1;
            uint8_t pause_breakpoint : 1;
            uint8_t pause_scanline_break : 1;
            uint8_t pause_interruption_break : 1;
            uint8_t pause_memory_break : 1;
        };
        uint8_t paused;
    };

    union {
        struct {
            uint8_t step_frame : 1;
            uint8_t step_scanline : 1;
            uint8_t step_instruction : 1;
        };
        uint8_t step;
    };

    bool skip;
    bool unlimited_fps;

    // interrupts_t *ints;

    // bool hide_bg;
    // bool hide_sprites;
    // bool sound_mute;
    // bool show_grid;
    bool crop_display;
    // bool show_attributes;

    // struct {
    //    bool pulse1;
    //    bool pulse2;
    //    bool noise;
    //    bool dmc;
    //    bool triangle;
    //} channel_mute;

    const bool quiet;
    const bool enable_disas;
    const union {
        struct {
            uint8_t enable_console : 1;
        };
        uint8_t enable_debugger;
    };

    uint64_t sound_overruns;
} inspector_t;

typedef enum inspector_mode_flag {
    INSPECTOR_MODE_CONSOLE = 0x01,
    INSPECTOR_MODE_NAMETABLE = 0x02,
    INSPECTOR_MODE_PATTERN = 0x04,
    INSPECTOR_MODE_PALETTE = 0x08,
} inspector_mode_flag_t;

typedef enum inspector_event {
    INSPECTOR_EVENT_QUIT = 1,
    INSPECTOR_EVENT_RESET,
    INSPECTOR_EVENT_PAUSE,
    INSPECTOR_EVENT_HIDE_BG,
    INSPECTOR_EVENT_HIDE_SPRITES,
    INSPECTOR_EVENT_SHOW_GRID,
    INSPECTOR_EVENT_SHOW_ATTRIBUTES,
    INSPECTOR_EVENT_CROP_DISPLAY,
    INSPECTOR_EVENT_STEP_FRAME,
    INSPECTOR_EVENT_STEP_SCANLINE,
    INSPECTOR_EVENT_STEP_INSTRUCTION,
    INSPECTOR_EVENT_SOUND_MUTE,
    INSPECTOR_EVENT_ABORT,
    INSPECTOR_EVENT_UNLIMITED_FPS,
} inspector_event_t;

int inspector_on_event(inspector_t *self, inspector_event_t e, bool on);
