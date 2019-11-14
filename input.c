#include "input.h"
#include "bus_internal.h"
#include "input_event_emitter.h"
#include "playlog.h"

typedef struct input {
    bus_ref shared_bus;
    input_event_emitter_t *shared_emitter;
} input_t;

void input_control_joypad(input_ref self, int joypad_idx, bool on,
                          input_event_control_joypad_button_t button)
{
    // GUARD(joypad_idx < 2);

    joypad_reg_t *reg = &self->shared_bus->joypads[joypad_idx].reg;

    switch (button) {
    case INPUT_EVENT_CONTROL_JOYPAD_BUTTON_A:
        reg->a = on;
        break;
    case INPUT_EVENT_CONTROL_JOYPAD_BUTTON_B:
        reg->b = on;
        break;
    case INPUT_EVENT_CONTROL_JOYPAD_BUTTON_START:
        reg->start = on;
        break;
    case INPUT_EVENT_CONTROL_JOYPAD_BUTTON_SELECT:
        reg->select = on;
        break;
    case INPUT_EVENT_CONTROL_JOYPAD_BUTTON_UP:
        reg->up = on;
        break;
    case INPUT_EVENT_CONTROL_JOYPAD_BUTTON_LEFT:
        reg->left = on;
        break;
    case INPUT_EVENT_CONTROL_JOYPAD_BUTTON_DOWN:
        reg->down = on;
        break;
    case INPUT_EVENT_CONTROL_JOYPAD_BUTTON_RIGHT:
        reg->right = on;
        break;
    }

    input_event_emit_control_joypad(self->shared_emitter, joypad_idx, on,
                                    button);
}

void input_set_controls(input_ref self, uint8_t system, joypad_reg_t joypad1,
                        joypad_reg_t joypad2)
{
    self->shared_bus->joypads[0].reg = joypad1;
    self->shared_bus->joypads[1].reg = joypad2;

    if (system & PLAYLOG_SYSTEM_RESET) {
        bus_assert_rst(self->shared_bus);
    }

    input_event_emit_set_controls(self->shared_emitter, system, joypad1,
                                  joypad2);
}

void input_destroy(input_ref self)
{
    if (self) {
        FREE(self);
    }
}

input_ref input_create(input_event_emitter_t *emitter)
{
    input_ref self = NULL;

    TALLOC(self);
    GUARD(self->shared_emitter = emitter);

    return self;
error:
    input_destroy(self);
    return NULL;
}

int input_set_bus(input_ref self, bus_ref bus)
{
    self->shared_bus = bus;

    return SUCCESS;
}
