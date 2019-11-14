#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "display.h"
#include "utils.h"

typedef struct display {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Rect crop;

    int width;
    int height;
    char *name;
    uint32_t *pixels;
} display_t;

display_t *display_create(const char *name, int window_width, int window_height,
                          int buffer_width, int buffer_height)
{
    display_t *self = NULL;

    TALLOC(self);

    self->width = buffer_width;
    self->height = buffer_height;
    GUARD(self->name = strndup(name, 256));

    GUARD(SDL_InitSubSystem(SDL_INIT_VIDEO) == 0);

    GUARD(self->window = SDL_CreateWindow(
              self->name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
              window_width, window_height,
              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE));

    TALLOCS(self->pixels, buffer_width * buffer_height);

    GUARD(self->renderer =
              SDL_CreateRenderer(self->window, -1, SDL_RENDERER_ACCELERATED));

    GUARD(self->texture = SDL_CreateTexture(
              self->renderer, SDL_PIXELFORMAT_RGB888,
              SDL_TEXTUREACCESS_STREAMING, self->width, self->height));

    GUARD(display_nocrop(self));

    return self;
error:
    display_report_error();
    return NULL;
}

void display_destroy(display_t *self)
{
    if (self) {
        SDL_DestroyTexture(self->texture);
        self->texture = NULL;
        SDL_DestroyRenderer(self->renderer);
        self->renderer = NULL;
        FREE(self->pixels);
        SDL_DestroyWindow(self->window);
        FREE(self->name);
    }
}

int display_update(display_t *self)
{
    GUARD(self);

    SDL_RenderClear(self->renderer);
    SDL_UpdateTexture(self->texture, NULL, self->pixels, self->width * 4);
    SDL_RenderCopy(self->renderer, self->texture, &self->crop, NULL);
    SDL_RenderPresent(self->renderer);

    return SUCCESS;
error:
    display_report_error();
    return NG;
}

int display_set_title(display_t *self, const char *title)
{
    SDL_SetWindowTitle(self->window, title);

    return SUCCESS;
}

int display_reset_title(display_t *self)
{
    SDL_SetWindowTitle(self->window, self->name);

    return SUCCESS;
}

int display_crop(display_t *self, uint16_t left, uint16_t right, uint16_t upper,
                 uint16_t lower)
{
    GUARD(self);
    GUARD(left + right < self->width);
    GUARD(upper + lower < self->height);

    self->crop = (SDL_Rect){
        .x = left,
        .w = self->width - (right + left),
        .y = upper,
        .h = self->height - (upper + lower),
    };

    return SUCCESS;
error:
    return NG;
}

int display_nocrop(display_t *self)
{
    return display_crop(self, 0, 0, 0, 0);
}

uint32_t *display_get_argb(display_ref self)
{
    return self->pixels;
}

void display_report_error(void)
{
    const char *str = SDL_GetError();
    if (strlen(str) > 1) {
        ERROR("SDL Error: %s\n", SDL_GetError());
        SDL_ClearError();
    }
}
