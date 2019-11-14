#pragma once

#include "input_event_emitter.h"
#include "joypad.h"
#include "utils.h"
typedef struct input *input_ref;
#include "bus.h"

void input_destroy(input_ref self);
input_ref input_create(input_event_emitter_t *emitter);
int input_set_bus(input_ref self, bus_ref bus);
void input_control_joypad(input_ref self, int joypad_idx, bool on,
                          input_event_control_joypad_button_t button);

void input_set_controls(input_ref self, uint8_t system, joypad_reg_t joypad1,
                        joypad_reg_t joypad2);
