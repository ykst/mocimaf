#include "inspector.h"
#include "utils.h"

int inspector_on_event(inspector_t *self, inspector_event_t e, bool on)
{
    // ignore keyup events
    if (!on) {
        return SUCCESS;
    }

    switch (e) {
    case INSPECTOR_EVENT_QUIT:
        self->quit = true;
        break;
    case INSPECTOR_EVENT_PAUSE:
        if (self->paused) {
            self->paused = 0;
        } else {
            self->pause_frame = 1;
        }
        break;
        // case INSPECTOR_EVENT_RESET:
        // interrupts_assert_rst(self->ints);
        // break;
        /*
            case INSPECTOR_EVENT_SHOW_GRID:
                negate(&self->show_grid);
                break;
            case INSPECTOR_EVENT_SHOW_ATTRIBUTES:
                negate(&self->show_attributes);
                break;
            case INSPECTOR_EVENT_CROP_DISPLAY:
                negate(&self->crop_display);
                break;
            case INSPECTOR_EVENT_HIDE_BG:
                negate(&self->hide_bg);
                break;
            case INSPECTOR_EVENT_HIDE_SPRITES:
                negate(&self->hide_sprites);
                break;
            case INSPECTOR_EVENT_SOUND_MUTE:
                negate(&self->sound_mute);
                break;
                */
    case INSPECTOR_EVENT_UNLIMITED_FPS:
        negate(&self->unlimited_fps);
        break;
    case INSPECTOR_EVENT_ABORT:
        THROW("aborted by inspector");
        break;
    case INSPECTOR_EVENT_STEP_FRAME:
        if (self->paused) {
            self->step_frame = 1;
            self->paused = 0;
            self->pause_frame = 1;
        }
        break;
    case INSPECTOR_EVENT_STEP_SCANLINE:
        if (self->paused) {
            self->step_scanline = 1;
            self->paused = 0;
            self->pause_scanline = 1;
        }
        break;
    case INSPECTOR_EVENT_STEP_INSTRUCTION:
        if (self->paused) {
            self->step_instruction = 1;
            self->paused = 0;
            self->pause_instruction = 1;
        }
        break;
    default:
        DIE("Unknown event: %d", e);
    }
    return SUCCESS;
error:
    return NG;
}
