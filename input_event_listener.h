// *** WARNING ***
// This file is generated by genevents.rb
// Keep untouched or ruin the abstraction
#pragma once
#include "utils.h"
#include "joypad.h"

typedef void (*input_event_callback_on_set_controls_t)(void *context, uint8_t system, joypad_reg_t joypad1, joypad_reg_t joypad2);

typedef enum input_event_control_joypad_idx {
    INPUT_EVENT_CONTROL_JOYPAD_IDX_PLAYER1,
    INPUT_EVENT_CONTROL_JOYPAD_IDX_PLAYER2,
} input_event_control_joypad_idx_t;

typedef enum input_event_control_joypad_button {
    INPUT_EVENT_CONTROL_JOYPAD_BUTTON_A,
    INPUT_EVENT_CONTROL_JOYPAD_BUTTON_B,
    INPUT_EVENT_CONTROL_JOYPAD_BUTTON_START,
    INPUT_EVENT_CONTROL_JOYPAD_BUTTON_SELECT,
    INPUT_EVENT_CONTROL_JOYPAD_BUTTON_RIGHT,
    INPUT_EVENT_CONTROL_JOYPAD_BUTTON_UP,
    INPUT_EVENT_CONTROL_JOYPAD_BUTTON_LEFT,
    INPUT_EVENT_CONTROL_JOYPAD_BUTTON_DOWN,
} input_event_control_joypad_button_t;
typedef void (*input_event_callback_on_control_joypad_t)(void *context, input_event_control_joypad_idx_t idx, bool is_on, input_event_control_joypad_button_t button);

typedef struct input_event_listener {
    void *context;
    // controller status playback for input injection. should be emitted on entering vblank.
    input_event_callback_on_set_controls_t on_set_controls;
    // pressed button
    input_event_callback_on_control_joypad_t on_control_joypad;
} input_event_listener_t;
