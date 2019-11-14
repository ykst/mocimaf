#include "sdl_driver_impl.h"
#include "audio.h"
#include "display.h"
#include "sdl_events.h"
#include <SDL.h>

typedef struct sdl_driver {
    // sdl_events_ref sdl_events;
    core_ref shared_core;
    display_ref display;
    audio_ref audio;

    bool mute_audio;
    bool crop_display;
    bool unlimited_fps;
    uint64_t prev_time;
} sdl_driver_t;

// http://wiki.nesdev.com/w/index.php/PPU_palettes
static uint32_t palette_lut_argb_2C02[0x40] = {
    [0x00] = 0x00545454, [0x01] = 0x00001e74, [0x02] = 0x00081090,
    [0x03] = 0x00300088, [0x04] = 0x00440064, [0x05] = 0x005c0030,
    [0x06] = 0x00540400, [0x07] = 0x003c1800, [0x08] = 0x00202a00,
    [0x09] = 0x00083a00, [0x0a] = 0x00004000, [0x0b] = 0x00003c00,
    [0x0c] = 0x0000323c, [0x0d] = 0x00000000, [0x0e] = 0x00000000,
    [0x0f] = 0x00000000, [0x10] = 0x00989698, [0x11] = 0x00084cc4,
    [0x12] = 0x003032ec, [0x13] = 0x005c1ee4, [0x14] = 0x008814b0,
    [0x15] = 0x00a01464, [0x16] = 0x00982220, [0x17] = 0x00783c00,
    [0x18] = 0x00545a00, [0x19] = 0x00287200, [0x1a] = 0x00087c00,
    [0x1b] = 0x00007628, [0x1c] = 0x00006678, [0x1d] = 0x00000000,
    [0x1e] = 0x00000000, [0x1f] = 0x00000000, [0x20] = 0x00eceeec,
    [0x21] = 0x004c9aec, [0x22] = 0x00787cec, [0x23] = 0x00b062ec,
    [0x24] = 0x00e454ec, [0x25] = 0x00ec58b4, [0x26] = 0x00ec6a64,
    [0x27] = 0x00d48820, [0x28] = 0x00a0aa00, [0x29] = 0x0074c400,
    [0x2a] = 0x004cd020, [0x2b] = 0x0038cc6c, [0x2c] = 0x0038b4cc,
    [0x2d] = 0x003c3c3c, [0x2e] = 0x00000000, [0x2f] = 0x00000000,
    [0x30] = 0x00eceeec, [0x31] = 0x00a8ccec, [0x32] = 0x00bcbcec,
    [0x33] = 0x00d4b2ec, [0x34] = 0x00ecaeec, [0x35] = 0x00ecaed4,
    [0x36] = 0x00ecb4b0, [0x37] = 0x00e4c490, [0x38] = 0x00ccd278,
    [0x39] = 0x00b4de78, [0x3a] = 0x00a8e290, [0x3b] = 0x0098e2b4,
    [0x3c] = 0x00a0d6e4, [0x3d] = 0x00a0a2a0, [0x3e] = 0x00000000,
    [0x3f] = 0x00000000,
};

void _map_joypad_input(sdl_driver_ref self, SDL_Event e)
{
    int config[][2] = {{SDLK_z, INPUT_EVENT_CONTROL_JOYPAD_BUTTON_A},
                       {SDLK_x, INPUT_EVENT_CONTROL_JOYPAD_BUTTON_B},
                       {SDLK_UP, INPUT_EVENT_CONTROL_JOYPAD_BUTTON_UP},
                       {SDLK_LEFT, INPUT_EVENT_CONTROL_JOYPAD_BUTTON_LEFT},
                       {SDLK_RIGHT, INPUT_EVENT_CONTROL_JOYPAD_BUTTON_RIGHT},
                       {SDLK_DOWN, INPUT_EVENT_CONTROL_JOYPAD_BUTTON_DOWN},
                       {SDLK_RETURN, INPUT_EVENT_CONTROL_JOYPAD_BUTTON_START},
                       {SDLK_RSHIFT, INPUT_EVENT_CONTROL_JOYPAD_BUTTON_SELECT},
                       {}};

    SDL_Keycode key = e.key.keysym.sym;

    for (int i = 0; config[i][0] != 0; ++i) {
        if (key == config[i][0]) {
            input_control_joypad(self->shared_core->input, 0,
                                 e.type == SDL_KEYDOWN, config[i][1]);
        }
    }
}

typedef enum sdl_driver_control_input {
    SDL_DRIVER_CONTROL_INPUT_QUIT,
    SDL_DRIVER_CONTROL_INPUT_ABORT,
    SDL_DRIVER_CONTROL_INPUT_RESET,

    SDL_DRIVER_CONTROL_TOGGLE_PAUSE,
    SDL_DRIVER_CONTROL_STEP_FRAME,

    SDL_DRIVER_CONTROL_TOGGLE_CROP,
    SDL_DRIVER_CONTROL_TOGGLE_UNLIMITED_FPS,

    SDL_DRIVER_CONTROL_TOGGLE_MUTE_AUDIO,
    SDL_DRIVER_CONTROL_TOGGLE_MUTE_PULSE1,
    SDL_DRIVER_CONTROL_TOGGLE_MUTE_PULSE2,
    SDL_DRIVER_CONTROL_TOGGLE_MUTE_NOISE,
    SDL_DRIVER_CONTROL_TOGGLE_MUTE_DMC,
    SDL_DRIVER_CONTROL_TOGGLE_MUTE_TRIANGLE,
    SDL_DRIVER_CONTROL_TOGGLE_HIDE_SPRITES,
    SDL_DRIVER_CONTROL_TOGGLE_HIDE_BACKGROUND,

    SDL_DRIVER_CONTROL_TOGGLE_DEBUG_GRID,
    SDL_DRIVER_CONTROL_TOGGLE_DEBUG_ATTRIBUTES,
} sdl_driver_control_input_t;

static void _handle_control_input(sdl_driver_ref self,
                                  sdl_driver_control_input_t input)
{
    switch (input) {
    case SDL_DRIVER_CONTROL_INPUT_QUIT:
        core_quit(self->shared_core);
        break;
    case SDL_DRIVER_CONTROL_INPUT_ABORT:
        THROW("aborted by inspector");
        break;
    case SDL_DRIVER_CONTROL_INPUT_RESET:
        core_reset(self->shared_core);
        break;
    case SDL_DRIVER_CONTROL_TOGGLE_PAUSE:
        core_toggle_pause(self->shared_core);
        break;
    case SDL_DRIVER_CONTROL_STEP_FRAME:
        break;

    case SDL_DRIVER_CONTROL_TOGGLE_CROP:
        negate(&self->crop_display);
        if (self->crop_display) {
            // PocketNES Safe Area
            display_crop(self->display, 8, 8, 16, 12);
        } else {
            display_nocrop(self->display);
        }
        break;
    case SDL_DRIVER_CONTROL_TOGGLE_UNLIMITED_FPS:
        negate(&self->unlimited_fps);
        break;

    case SDL_DRIVER_CONTROL_TOGGLE_MUTE_AUDIO:
        audio_pause(self->audio, negate(&self->mute_audio));
        break;
    case SDL_DRIVER_CONTROL_TOGGLE_MUTE_PULSE1:
        core_toggle_mute_pulse1(self->shared_core);
        break;
    case SDL_DRIVER_CONTROL_TOGGLE_MUTE_PULSE2:
        core_toggle_mute_pulse2(self->shared_core);
        break;
    case SDL_DRIVER_CONTROL_TOGGLE_MUTE_NOISE:
        core_toggle_mute_noise(self->shared_core);
        break;
    case SDL_DRIVER_CONTROL_TOGGLE_MUTE_DMC:
        core_toggle_mute_dmc(self->shared_core);
        break;
    case SDL_DRIVER_CONTROL_TOGGLE_MUTE_TRIANGLE:
        core_toggle_mute_triangle(self->shared_core);
        break;
    case SDL_DRIVER_CONTROL_TOGGLE_HIDE_SPRITES:
        core_toggle_hide_sprites(self->shared_core);
        break;
    case SDL_DRIVER_CONTROL_TOGGLE_HIDE_BACKGROUND:
        core_toggle_hide_background(self->shared_core);
        break;

    case SDL_DRIVER_CONTROL_TOGGLE_DEBUG_GRID:
        break;
    case SDL_DRIVER_CONTROL_TOGGLE_DEBUG_ATTRIBUTES:
        break;
    }
}

void _map_control_input(sdl_driver_ref self, SDL_Event e)
{
    int config[][2] =
        (int[][2]){{SDLK_q, SDL_DRIVER_CONTROL_INPUT_QUIT},
                   {SDLK_r, SDL_DRIVER_CONTROL_INPUT_RESET},
                   {SDLK_g, SDL_DRIVER_CONTROL_TOGGLE_DEBUG_GRID},
                   {SDLK_t, SDL_DRIVER_CONTROL_TOGGLE_DEBUG_ATTRIBUTES},
                   {SDLK_d, SDL_DRIVER_CONTROL_TOGGLE_CROP},
                   {SDLK_b, SDL_DRIVER_CONTROL_TOGGLE_HIDE_BACKGROUND},
                   {SDLK_e, SDL_DRIVER_CONTROL_INPUT_ABORT},
                   {SDLK_p, SDL_DRIVER_CONTROL_TOGGLE_PAUSE},
                   {SDLK_s, SDL_DRIVER_CONTROL_TOGGLE_HIDE_SPRITES},
                   {SDLK_u, SDL_DRIVER_CONTROL_TOGGLE_UNLIMITED_FPS},
                   {SDLK_a, SDL_DRIVER_CONTROL_TOGGLE_MUTE_AUDIO},
                   {SDLK_1, SDL_DRIVER_CONTROL_TOGGLE_MUTE_PULSE1},
                   {SDLK_2, SDL_DRIVER_CONTROL_TOGGLE_MUTE_PULSE2},
                   {SDLK_3, SDL_DRIVER_CONTROL_TOGGLE_MUTE_TRIANGLE},
                   {SDLK_4, SDL_DRIVER_CONTROL_TOGGLE_MUTE_NOISE},
                   {SDLK_5, SDL_DRIVER_CONTROL_TOGGLE_MUTE_DMC},
                   {SDLK_DOWN, SDL_DRIVER_CONTROL_STEP_FRAME},
                   {}};

    if (e.type == SDL_KEYDOWN) {
        SDL_Keycode key = e.key.keysym.sym;

        for (int i = 0; config[i][0] != 0; ++i) {
            if (key == config[i][0]) {
                _handle_control_input(self, config[i][1]);
            }
        }
    }
}

static inline void _load_input_config(sdl_driver_ref self)
{

    /*
    sdl_keyevents_load(self->sdl_events, &self->shared_core->bus->joypads[0],
                       (sdl_keyevent_handler_t)joypad_on_event,
                       (int[][2]){
                           {SDLK_z, JOYPAD_EVENT_A},
                                  {SDLK_x, JOYPAD_EVENT_B},
                                  {SDLK_UP, JOYPAD_EVENT_UP},
                                  {SDLK_LEFT, JOYPAD_EVENT_LEFT},
                                  {SDLK_RIGHT, JOYPAD_EVENT_RIGHT},
                                  {SDLK_DOWN, JOYPAD_EVENT_DOWN},
                                  {SDLK_RETURN, JOYPAD_EVENT_START},
                                  {SDLK_RSHIFT, JOYPAD_EVENT_SELECT},
                                  {}});
                                  */
}

static inline void _poll_events(sdl_driver_ref self, bool poll_joypad)
{
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            core_quit(self->shared_core);
            break;
        }
        if (poll_joypad) {
            _map_joypad_input(self, e);
        }
        _map_control_input(self, e);
    }
}

void sdl_driver_destroy(sdl_driver_ref self)
{
    if (self) {
        // sdl_events_destroy(self->sdl_events);
        display_destroy(self->display);
        audio_destroy(self->audio);
        FREE(self);
        SDL_Quit();
    }
}

void sdl_driver_on_core_pause(sdl_driver_ref self, core_ref core)
{
    audio_pause(self->audio, true);
}

void sdl_driver_on_core_resume(sdl_driver_ref self, core_ref core)
{
    if (!self->mute_audio) {
        audio_pause(self->audio, false);
    }
}

sdl_driver_ref sdl_driver_create(core_ref core, args_t *args)
{
    sdl_driver_ref self = NULL;
    TALLOC(self);
    GUARD(self->shared_core = core);

    GUARD(SDL_Init(0) == 0);
    GUARD(SDL_InitSubSystem(SDL_INIT_EVENTS) == 0);

    // GUARD(self->sdl_events = sdl_events_create());

    GUARD(self->display =
              display_create(core->shared_ines->name, 584, 480, 256, 240));
    GUARD(self->audio = audio_create(core->apu));

    audio_pause(self->audio, false);

    _load_input_config(self);

    return self;
error:
    sdl_driver_destroy(self);
    return NULL;
}

void _adjust_framerate(sdl_driver_ref self)
{
    uint64_t current_time = SDL_GetPerformanceCounter();
    double delta_ms = (current_time - self->prev_time) * 1000.0 /
                      (double)SDL_GetPerformanceFrequency();

    // if (!inspector.unlimited_fps && !inspector.paused) {
    if (delta_ms < 16) {
        // usleep((1000 / 60.0 - delta_ms) * 1000);
        SDL_Delay(16 - delta_ms);
    }

    while (audio_needs_video_sync(self->audio)) {
        usleep(200);
        // SDL_Delay(1);
    }

    self->prev_time = SDL_GetPerformanceCounter();
}

void sdl_driver_on_ppu_frame(sdl_driver_ref self, ppu_ref ppu, uint64_t frames)
{
    _poll_events(self, true);
    // if (!self->shared_core->paused)
    display_update(self->display);
    if (!self->unlimited_fps) {
        _adjust_framerate(self);
    }
}

void sdl_driver_on_core_poll_in_pause(sdl_driver_ref self, core_ref core)
{
    _poll_events(self, false);

    // dim the previous image to highlight newly updated scanlines.
    // the difference is visible in sub-frame stepping mode.
    if (core->ppu->scanline.y == 0) {
        uint32_t *argb = display_get_argb(self->display);
        for (int i = 0; i < 256 * 240; ++i) {
            argb[i] = (argb[i] >> 2) & ~0xC0C0C0C0;
        }
    }

    display_update(self->display);
}

void sdl_driver_on_core_start(sdl_driver_ref self, core_ref core)
{
    _poll_events(self, true);
}

void sdl_driver_on_ppu_scanline(sdl_driver_ref self, ppu_ref ppu, int line,
                                const uint16_t *dots)
{
    uint32_t *argb = display_get_argb(self->display);

    //        if (!self->args->no_trace && !self->args->quiet) {
    //        }

    for (int i = 0; i < 256; ++i) {
        argb[256 * line + i] = palette_lut_argb_2C02[dots[i] & 0x3F];
    }
}
/*
                if (inspector.crop_display) {
                    // PocketNES Safe Area
                    GUARD(display_crop(display, 8, 8, 16, 12));
                } else {
                    GUARD(display_nocrop(display));
                }
                */

void sdl_driver_on_apu_level30(sdl_driver_ref self, apu_ref apu, float level)
{
    audio_feed_level30(self->audio, level);
}
