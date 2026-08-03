#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *title;
    const char *subtitle;
    const char *items[256];
    size_t item_count;
    size_t grid_columns;
    const char *(*item_text)(size_t index, void *userdata);
    const char *footer;
    const char *now_playing_title;
    const char *now_playing_country;
    void (*on_input)(uint32_t trigger, size_t *selected, char *status,
                     size_t status_size, void *userdata);
    void (*on_tick)(size_t *selected, char *status, size_t status_size, void *userdata);
    void (*on_shutdown)(void *userdata);
    void *userdata;
    const unsigned char *tv_background_jpeg;
    size_t tv_background_size;
    const unsigned char *drc_background_jpeg;
    size_t drc_background_size;
    const unsigned char *tv_watermark_rgba;
    size_t tv_watermark_size;
    int tv_watermark_width, tv_watermark_height;
    const unsigned char *drc_watermark_rgba;
    size_t drc_watermark_size;
    int drc_watermark_width, drc_watermark_height;
} ToribioApp;

int toribio_run(ToribioApp *app);
