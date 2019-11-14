#pragma once

typedef struct display *display_ref;

display_ref display_create(const char *name, int window_width,
                           int window_height, int buffer_width,
                           int buffer_height);

void display_destroy(display_ref self);
int display_update(display_ref self);
void display_report_error(void);
int display_crop(display_ref self, uint16_t left, uint16_t right,
                 uint16_t upper, uint16_t lower);
int display_nocrop(display_ref self);

int display_reset_title(display_ref self);
int display_set_title(display_ref self, const char *title);

uint32_t *display_get_argb(display_ref self);
