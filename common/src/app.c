#include <toribio/app.h>
#include <coreinit/screen.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/time.h>
#include <coreinit/thread.h>
#include <vpad/input.h>
#include <padscore/kpad.h>
#include <padscore/wpad.h>
#include <whb/proc.h>
#include <turbojpeg.h>
#include <stdio.h>
#include <string.h>

static void *tv_buffer, *drc_buffer;
static void *tv_background, *drc_background;
static uint32_t tv_single_size, drc_single_size;
static unsigned tv_work, drc_work;

static void draw_line(uint32_t *pixels, int pitch, int width, int height,
                      int x1, int y1, int x2, int y2, uint32_t colour) {
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > width) x2 = width;
    if (y2 > height) y2 = height;
    for (int y = y1; y < y2; ++y)
        for (int x = x1; x < x2; ++x) pixels[y * pitch + x] = colour;
}

static const uint8_t *glyph_rows(char character) {
    static const uint8_t letters[26][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
        {14,4,4,4,4,4,14}, {7,2,2,2,2,18,12},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}
    };
    static const uint8_t digits[10][7] = {
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14}
    };
    static const uint8_t blank[7] = {0,0,0,0,0,0,0};
    static const uint8_t question[7] = {14,17,1,2,4,0,4};
    static const uint8_t period[7] = {0,0,0,0,0,6,6};
    static const uint8_t comma[7] = {0,0,0,0,6,6,4};
    static const uint8_t colon[7] = {0,6,6,0,6,6,0};
    static const uint8_t semicolon[7] = {0,6,6,0,6,6,4};
    static const uint8_t dash[7] = {0,0,0,31,0,0,0};
    static const uint8_t underscore[7] = {0,0,0,0,0,0,31};
    static const uint8_t slash[7] = {1,2,2,4,8,8,16};
    static const uint8_t backslash[7] = {16,8,8,4,2,2,1};
    static const uint8_t pipe[7] = {4,4,4,4,4,4,4};
    static const uint8_t plus[7] = {0,4,4,31,4,4,0};
    static const uint8_t equal[7] = {0,0,31,0,31,0,0};
    static const uint8_t less[7] = {1,2,4,8,4,2,1};
    static const uint8_t greater[7] = {16,8,4,2,4,8,16};
    static const uint8_t left_paren[7] = {2,4,8,8,8,4,2};
    static const uint8_t right_paren[7] = {8,4,2,2,2,4,8};
    static const uint8_t left_bracket[7] = {14,8,8,8,8,8,14};
    static const uint8_t right_bracket[7] = {14,2,2,2,2,2,14};
    static const uint8_t apostrophe[7] = {6,6,4,0,0,0,0};
    static const uint8_t quote[7] = {10,10,0,0,0,0,0};
    static const uint8_t exclamation[7] = {4,4,4,4,4,0,4};
    static const uint8_t ampersand[7] = {12,18,20,8,21,18,13};
    if (character >= 'a' && character <= 'z') character -= ('a' - 'A');
    if (character >= 'A' && character <= 'Z') return letters[character - 'A'];
    if (character >= '0' && character <= '9') return digits[character - '0'];
    switch (character) {
        case ' ': return blank; case '.': return period; case ',': return comma;
        case ':': return colon; case ';': return semicolon; case '-': return dash;
        case '_': return underscore; case '/': return slash; case '\\': return backslash;
        case '|': return pipe; case '+': return plus; case '=': return equal;
        case '<': return less; case '>': return greater; case '(': return left_paren;
        case ')': return right_paren; case '[': return left_bracket; case ']': return right_bracket;
        case '\'': return apostrophe; case '"': return quote; case '!': return exclamation;
        case '&': return ampersand; default: return question;
    }
}

static int bitmap_text_width(const char *text, int scale) {
    size_t length = text ? strlen(text) : 0;
    return length ? (int)(length * 6 * scale - scale) : 0;
}

static void draw_glyph(uint32_t *pixels, int pitch, int width, int height,
                       int x, int y, char character, int scale, uint32_t colour) {
    const uint8_t *rows = glyph_rows(character);
    for (int row = 0; row < 7; ++row)
        for (int column = 0; column < 5; ++column)
            if (rows[row] & (1u << (4 - column)))
                draw_line(pixels, pitch, width, height,
                          x + column * scale, y + row * scale,
                          x + (column + 1) * scale, y + (row + 1) * scale, colour);
}

static void draw_bitmap_text(uint32_t *pixels, int pitch, int width, int height,
                             int x, int y, const char *text, int scale) {
    if (!text) return;
    int shadow = scale > 2 ? 2 : 1;
    for (size_t i = 0; text[i]; ++i)
        draw_glyph(pixels, pitch, width, height, x + (int)i * 6 * scale + shadow,
                   y + shadow, text[i], scale, 0x100A08FF);
    for (size_t i = 0; text[i]; ++i)
        draw_glyph(pixels, pitch, width, height, x + (int)i * 6 * scale,
                   y, text[i], scale, 0xFFF4D6FF);
}

static void draw_bitmap_text_clipped(uint32_t *pixels, int pitch, int width, int height,
                                     int x, int y, const char *text, int scale,
                                     int maximum_width) {
    char clipped[128];
    if (!text || maximum_width <= 0) return;
    size_t maximum = (size_t)((maximum_width + scale) / (6 * scale));
    if (maximum > sizeof(clipped) - 1) maximum = sizeof(clipped) - 1;
    size_t length = strlen(text);
    if (length <= maximum) memcpy(clipped, text, length + 1);
    else {
        memcpy(clipped, text, maximum);
        if (maximum >= 3) memcpy(clipped + maximum - 3, "...", 3);
        clipped[maximum] = 0;
    }
    draw_bitmap_text(pixels, pitch, width, height, x, y, clipped, scale);
}

static void shade_rect(uint32_t *pixels, int pitch, int width, int height,
                       int x, int y, int w, int h) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > width) w = width - x;
    if (y + h > height) h = height - y;
    for (int py = y; py < y + h; ++py) {
        uint32_t *row = pixels + py * pitch;
        for (int px = x; px < x + w; ++px) {
            uint32_t colour = row[px];
            uint32_t r = ((colour >> 24) & 0xff) * 2 / 3;
            uint32_t g = ((colour >> 16) & 0xff) * 2 / 3;
            uint32_t b = ((colour >> 8) & 0xff) * 2 / 3;
            row[px] = (r << 24) | (g << 16) | (b << 8) | 0xff;
        }
    }
}

static void blend_watermark(uint32_t *pixels, int pitch, int width, int height,
                            const unsigned char *source, size_t source_size,
                            int source_width, int source_height) {
    if (!source || source_width <= 0 || source_height <= 0 ||
        source_size < (size_t)source_width * (size_t)source_height * 4) return;
    int margin = width >= 1000 ? 12 : 8;
    int start_x = width - source_width - margin;
    int start_y = height - source_height - margin;
    int padding = width >= 1000 ? 7 : 5;
    int edge = width >= 1000 ? 3 : 2;
    draw_line(pixels, pitch, width, height,
              start_x - padding, start_y - padding,
              start_x + source_width + padding, start_y + source_height + padding,
              0xE9DFC9FF);
    draw_line(pixels, pitch, width, height,
              start_x - padding, start_y - padding,
              start_x + source_width + padding, start_y - padding + edge,
              0x5A321DFF);
    draw_line(pixels, pitch, width, height,
              start_x - padding, start_y - padding,
              start_x - padding + edge, start_y + source_height + padding,
              0x5A321DFF);
    draw_line(pixels, pitch, width, height,
              start_x - padding, start_y + source_height + padding - edge,
              start_x + source_width + padding, start_y + source_height + padding,
              0xA67845FF);
    draw_line(pixels, pitch, width, height,
              start_x + source_width + padding - edge, start_y - padding,
              start_x + source_width + padding, start_y + source_height + padding,
              0xA67845FF);
    for (int y = 0; y < source_height; ++y) {
        for (int x = 0; x < source_width; ++x) {
            const unsigned char *rgba = source + ((size_t)y * source_width + x) * 4;
            unsigned alpha = rgba[3];
            if (!alpha) continue;
            uint32_t destination = pixels[(start_y + y) * pitch + start_x + x];
            unsigned inverse = 255 - alpha;
            unsigned red = (rgba[0] * alpha + ((destination >> 24) & 0xff) * inverse + 127) / 255;
            unsigned green = (rgba[1] * alpha + ((destination >> 16) & 0xff) * inverse + 127) / 255;
            unsigned blue = (rgba[2] * alpha + ((destination >> 8) & 0xff) * inverse + 127) / 255;
            pixels[(start_y + y) * pitch + start_x + x] =
                (red << 24) | (green << 16) | (blue << 8) | 0xff;
        }
    }
}

static void draw_pressed_button(uint32_t *pixels, int pitch, int width, int height,
                                size_t selected) {
    const float scale = (float)width / 1280.0f;
    int column = (int)(selected % 2);
    int row = (int)(selected / 2);
    int x = (int)((column ? 371 : 52) * scale);
    int y = (int)((68 + row * 102) * scale);
    int w = (int)((column ? 303 : 294) * scale);
    int h = (int)(89 * scale);
    int inset = (int)(8 * scale);
    int edge = (int)(3 * scale);
    if (edge < 2) edge = 2;
    shade_rect(pixels, pitch, width, height, x + inset, y + inset,
               w - inset * 2, h - inset * 2);
    /* Bisel invertido: el pulsador seleccionado parece hundido. */
    draw_line(pixels, pitch, width, height, x, y, x + w, y + edge, 0x3A1609FF);
    draw_line(pixels, pitch, width, height, x, y, x + edge, y + h, 0x3A1609FF);
    draw_line(pixels, pitch, width, height, x, y + h - edge, x + w, y + h, 0xE0A14BFF);
    draw_line(pixels, pitch, width, height, x + w - edge, y, x + w, y + h, 0xE0A14BFF);
}

static void draw_station_panel(uint32_t *pixels, int pitch, int width, int height) {
    const float scale = (float)width / 1280.0f;
    int x = (int)(42 * scale), y = (int)(42 * scale);
    int w = (int)(665 * scale), h = (int)(630 * scale);
    int edge = (int)(5 * scale);
    if (edge < 3) edge = 3;
    draw_line(pixels, pitch, width, height, x, y, x + w, y + h, 0x21140FFF);
    draw_line(pixels, pitch, width, height, x, y, x + w, y + edge, 0xD49A55FF);
    draw_line(pixels, pitch, width, height, x, y, x + edge, y + h, 0xD49A55FF);
    draw_line(pixels, pitch, width, height, x, y + h - edge, x + w, y + h, 0x5A2915FF);
    draw_line(pixels, pitch, width, height, x + w - edge, y, x + w, y + h, 0x5A2915FF);
    int inner = edge + (int)(5 * scale);
    draw_line(pixels, pitch, width, height, x + inner, y + inner,
              x + w - inner, y + inner + edge, 0x4A2C20FF);
}

static unsigned detect_work_buffer(OSScreenID screen, void *base, uint32_t single) {
    unsigned char *bytes = base;
    memset(bytes, 0x11, single * 2);
    OSScreenClearBufferEx(screen, 0x123456FF);
    if (bytes[0] == 0x12 && bytes[1] == 0x34 && bytes[2] == 0x56) return 0;
    if (bytes[single] == 0x12 && bytes[single + 1] == 0x34 &&
        bytes[single + 2] == 0x56) return 1;
    return 0;
}

static const char *item_text(const ToribioApp *app, size_t index) {
    if (app->item_text) {
        const char *text = app->item_text(index, app->userdata);
        return text ? text : "";
    }
    return index < 256 && app->items[index] ? app->items[index] : "";
}

static uint32_t map_kpad_buttons(const KPADStatus *pad) {
    uint32_t source = pad->trigger;
    bool extended = false;
    if (pad->extensionType == WPAD_EXT_PRO_CONTROLLER) { source = pad->pro.trigger; extended = true; }
    else if (pad->extensionType == WPAD_EXT_CLASSIC || pad->extensionType == WPAD_EXT_MPLUS_CLASSIC) {
        source = pad->classic.trigger; extended = true;
    }
    uint32_t out = 0;
    if (extended) {
        if (source & WPAD_PRO_BUTTON_A) out |= VPAD_BUTTON_A;
        if (source & WPAD_PRO_BUTTON_B) out |= VPAD_BUTTON_B;
        if (source & WPAD_PRO_BUTTON_X) out |= VPAD_BUTTON_X;
        if (source & WPAD_PRO_BUTTON_Y) out |= VPAD_BUTTON_Y;
        if (source & WPAD_PRO_BUTTON_L) out |= VPAD_BUTTON_L;
        if (source & WPAD_PRO_BUTTON_R) out |= VPAD_BUTTON_R;
        if (source & WPAD_PRO_BUTTON_UP) out |= VPAD_BUTTON_UP;
        if (source & WPAD_PRO_BUTTON_DOWN) out |= VPAD_BUTTON_DOWN;
        if (source & WPAD_PRO_BUTTON_LEFT) out |= VPAD_BUTTON_LEFT;
        if (source & WPAD_PRO_BUTTON_RIGHT) out |= VPAD_BUTTON_RIGHT;
    } else {
        if (source & WPAD_BUTTON_A) out |= VPAD_BUTTON_A;
        if (source & WPAD_BUTTON_B) out |= VPAD_BUTTON_B;
        if (source & WPAD_BUTTON_1) out |= VPAD_BUTTON_X;
        if (source & WPAD_BUTTON_2) out |= VPAD_BUTTON_Y;
        if (source & WPAD_BUTTON_MINUS) out |= VPAD_BUTTON_L;
        if (source & WPAD_BUTTON_PLUS) out |= VPAD_BUTTON_R;
        if (source & WPAD_BUTTON_UP) out |= VPAD_BUTTON_UP;
        if (source & WPAD_BUTTON_DOWN) out |= VPAD_BUTTON_DOWN;
        if (source & WPAD_BUTTON_LEFT) out |= VPAD_BUTTON_LEFT;
        if (source & WPAD_BUTTON_RIGHT) out |= VPAD_BUTTON_RIGHT;
    }
    return out;
}

static uint32_t read_kpad_trigger(void) {
    uint32_t trigger = 0;
    for (int channel = 0; channel < 4; ++channel) {
        KPADStatus pad;
        if (KPADRead((KPADChan)channel, &pad, 1) > 0 && pad.error == KPAD_ERROR_OK)
            trigger |= map_kpad_buttons(&pad);
    }
    return trigger;
}

static void *decode_background(const unsigned char *jpeg, size_t size,
                               int expected_width, int expected_height, uint32_t allocation) {
    if (!jpeg || !size) return NULL;
    tjhandle decoder = tjInitDecompress();
    if (!decoder) return NULL;
    int width = 0, height = 0, subsamp = 0, colorspace = 0;
    if (tjDecompressHeader3(decoder, jpeg, (unsigned long)size, &width, &height,
                            &subsamp, &colorspace) < 0 ||
        width != expected_width || height != expected_height) {
        tjDestroy(decoder); return NULL;
    }
    void *pixels = MEMAllocFromDefaultHeapEx(allocation, 0x100);
    if (!pixels) { tjDestroy(decoder); return NULL; }
    memset(pixels, 0, allocation);
    int pitch = (int)(allocation / (uint32_t)expected_height);
    if (tjDecompress2(decoder, jpeg, (unsigned long)size, pixels, width, pitch, height,
                      TJPF_RGBX, TJFLAG_FASTDCT) < 0) {
        MEMFreeToDefaultHeap(pixels); pixels = NULL;
    }
    tjDestroy(decoder);
    return pixels;
}

static void draw_screen(OSScreenID screen, const ToribioApp *app,
                        size_t selected, const char *status) {
    char line[96];
    void *background = screen == SCREEN_TV ? tv_background : drc_background;
    void *base = screen == SCREEN_TV ? tv_buffer : drc_buffer;
    uint32_t single = screen == SCREEN_TV ? tv_single_size : drc_single_size;
    unsigned *work = screen == SCREEN_TV ? &tv_work : &drc_work;
    unsigned char *work_buffer = (unsigned char *)base + (*work * single);
    if (background) memcpy(work_buffer, background, single);
    else OSScreenClearBufferEx(screen, 0x071426FF);

    int width = screen == SCREEN_TV ? 1280 : 854;
    int height = screen == SCREEN_TV ? 720 : 480;
    int pitch = (int)(single / ((uint32_t)height * 4));
    uint32_t *pixels = (uint32_t *)work_buffer;
    const float ui_scale = (float)width / 1280.0f;
    const int text_scale = width >= 1000 ? 3 : 2;

    if (app->grid_columns > 1) {
        draw_pressed_button(pixels, pitch, width, height, selected);
        for (size_t i = 0; i < app->item_count; ++i) {
            int column = (int)(i % 2), row = (int)(i / 2);
            int button_x = (int)((column ? 371 : 52) * ui_scale);
            int button_y = (int)((68 + row * 102) * ui_scale);
            int button_width = (int)((column ? 303 : 294) * ui_scale);
            int button_height = (int)(89 * ui_scale);
            int country_scale = width >= 1000 ? 4 : 3;
            snprintf(line, sizeof(line), i == selected ? ">%s<" : "%s", item_text(app, i));
            int label_width = bitmap_text_width(line, country_scale);
            while (country_scale > 2 && label_width > button_width - (int)(18 * ui_scale)) {
                --country_scale;
                label_width = bitmap_text_width(line, country_scale);
            }
            int label_height = 7 * country_scale;
            draw_bitmap_text(pixels, pitch, width, height,
                             button_x + (button_width - label_width) / 2,
                             button_y + (button_height - label_height) / 2,
                             line, country_scale);
        }
    } else {
        draw_station_panel(pixels, pitch, width, height);
        int panel_left = (int)(60 * ui_scale);
        int panel_width = (int)(625 * ui_scale);
        draw_bitmap_text_clipped(pixels, pitch, width, height, panel_left,
                                 (int)(65 * ui_scale), app->title,
                                 text_scale, panel_width);
        draw_bitmap_text_clipped(pixels, pitch, width, height, panel_left,
                                 (int)(105 * ui_scale), app->subtitle,
                                 text_scale, panel_width);
        const size_t visible = 10;
        size_t start = selected >= visible ? selected - visible + 1 : 0;
        for (size_t row = 0; row < visible && start + row < app->item_count; ++row) {
            size_t i = start + row;
            snprintf(line, sizeof(line), "%c %03u/%03u %s", i == selected ? '>' : ' ',
                     (unsigned)(i + 1), (unsigned)app->item_count, item_text(app, i));
            draw_bitmap_text_clipped(pixels, pitch, width, height, panel_left,
                                     (int)((145 + (int)row * 44) * ui_scale), line,
                                     text_scale, panel_width);
        }
        draw_bitmap_text_clipped(pixels, pitch, width, height, panel_left,
                                 (int)(610 * ui_scale), status,
                                 text_scale, panel_width);
    }
    if (app->now_playing_title && app->now_playing_title[0]) {
        snprintf(line, sizeof(line), "SONANDO: %s", app->now_playing_title);
        draw_bitmap_text_clipped(pixels, pitch, width, height,
                                 (int)(52 * ui_scale), (int)(8 * ui_scale),
                                 line, text_scale, (int)(625 * ui_scale));
        snprintf(line, sizeof(line), "PAIS: %s",
                 app->now_playing_country ? app->now_playing_country : "");
        draw_bitmap_text_clipped(pixels, pitch, width, height,
                                 (int)(52 * ui_scale), (int)(36 * ui_scale),
                                 line, text_scale, (int)(625 * ui_scale));
    }
    if (screen == SCREEN_TV)
        blend_watermark(pixels, pitch, width, height, app->tv_watermark_rgba,
                        app->tv_watermark_size, app->tv_watermark_width,
                        app->tv_watermark_height);
    else
        blend_watermark(pixels, pitch, width, height, app->drc_watermark_rgba,
                        app->drc_watermark_size, app->drc_watermark_width,
                        app->drc_watermark_height);
    draw_bitmap_text_clipped(pixels, pitch, width, height, (int)(50 * ui_scale),
                             (int)(690 * ui_scale), app->footer,
                             text_scale, (int)(1180 * ui_scale));
    OSScreenFlipBuffersEx(screen);
    *work ^= 1;
}

int toribio_run(ToribioApp *app) {
    WHBProcInit(); OSScreenInit(); KPADInit();
    uint32_t tv_total = OSScreenGetBufferSizeEx(SCREEN_TV);
    uint32_t drc_total = OSScreenGetBufferSizeEx(SCREEN_DRC);
    tv_single_size = tv_total / 2; drc_single_size = drc_total / 2;
    tv_buffer = MEMAllocFromDefaultHeapEx(tv_total, 0x100);
    drc_buffer = MEMAllocFromDefaultHeapEx(drc_total, 0x100);
    if (!tv_buffer || !drc_buffer) return 2;
    OSScreenSetBufferEx(SCREEN_TV, tv_buffer); OSScreenSetBufferEx(SCREEN_DRC, drc_buffer);
    OSScreenEnableEx(SCREEN_TV, true); OSScreenEnableEx(SCREEN_DRC, true);
    tv_work = detect_work_buffer(SCREEN_TV, tv_buffer, tv_single_size);
    drc_work = detect_work_buffer(SCREEN_DRC, drc_buffer, drc_single_size);
    tv_background = decode_background(app->tv_background_jpeg, app->tv_background_size,
                                      1280, 720, tv_single_size);
    drc_background = decode_background(app->drc_background_jpeg, app->drc_background_size,
                                       854, 480, drc_single_size);

    size_t selected = 0;
    char status[96] = "Selecciona un pais";
    while (WHBProcIsRunning()) {
        VPADStatus input; VPADReadError error;
        uint32_t trigger = read_kpad_trigger();
        VPADRead(VPAD_CHAN_0, &input, 1, &error);
        if (error == VPAD_READ_SUCCESS) trigger |= input.trigger;
        if (trigger) {
            if (app->grid_columns > 1) {
                size_t cols = app->grid_columns;
                if ((trigger & VPAD_BUTTON_DOWN) && selected + cols < app->item_count) selected += cols;
                if ((trigger & VPAD_BUTTON_UP) && selected >= cols) selected -= cols;
                if ((trigger & VPAD_BUTTON_RIGHT) && selected % cols + 1 < cols && selected + 1 < app->item_count) selected++;
                if ((trigger & VPAD_BUTTON_LEFT) && selected % cols > 0) selected--;
            } else {
                if ((trigger & VPAD_BUTTON_DOWN) && selected + 1 < app->item_count) selected++;
                if ((trigger & VPAD_BUTTON_UP) && selected > 0) selected--;
            }
            if (app->on_input)
                app->on_input(trigger, &selected, status, sizeof(status), app->userdata);
            else if (trigger & VPAD_BUTTON_A)
                snprintf(status, sizeof(status), "Seleccion confirmada");
        }
        if (selected >= app->item_count && app->item_count) selected = app->item_count - 1;
        if (app->on_tick) app->on_tick(&selected, status, sizeof(status), app->userdata);
        draw_screen(SCREEN_TV, app, selected, status);
        draw_screen(SCREEN_DRC, app, selected, status);
        OSSleepTicks(OSMillisecondsToTicks(30));
    }
    if (app->on_shutdown) app->on_shutdown(app->userdata);
    KPADShutdown();
    if (tv_background) MEMFreeToDefaultHeap(tv_background);
    if (drc_background) MEMFreeToDefaultHeap(drc_background);
    OSScreenEnableEx(SCREEN_TV, false); OSScreenEnableEx(SCREEN_DRC, false);
    MEMFreeToDefaultHeap(tv_buffer); MEMFreeToDefaultHeap(drc_buffer);
    WHBProcShutdown(); return 0;
}
