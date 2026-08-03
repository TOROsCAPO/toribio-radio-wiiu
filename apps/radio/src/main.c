#include <toribio/app.h>
#include "radio_backend.h"
#include "catalog_backend.h"
#include "radio_bg_tv_jpg.h"
#include "radio_bg_drc_jpg.h"
#include "toribio_watermark_tv_rgba.h"
#include "toribio_watermark_drc_rgba.h"
#include <vpad/input.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { const char *name; const char *code; } Country;
static const Country countries[] = {
    {"Uruguay", "UY"}, {"Argentina", "AR"}, {"Brasil", "BR"},
    {"Peru", "PE"}, {"Colombia", "CO"}, {"Chile", "CL"},
    {"Ecuador", "EC"}, {"Bolivia", "BO"}, {"Paraguay", "PY"},
    {"Venezuela", "VE"}, {"Espana", "ES"}, {"Mexico", "MX"}
};

typedef enum { MODE_COUNTRIES, MODE_LOADING, MODE_CATALOG_ERROR, MODE_BANDS, MODE_STATIONS } UiMode;
typedef enum { FILTER_ALL, FILTER_FM, FILTER_AM, FILTER_OTHER } BandFilter;
typedef struct {
    ToribioApp *app;
    RadioBackend radio;
    CatalogBackend catalog;
    UiMode mode;
    char subtitle[96];
    char band_labels[4][24];
    char now_playing[72];
    char now_playing_country[24];
    size_t country_index;
    size_t *station_indices;
    size_t station_index_capacity;
} RadioUi;
static RadioUi ui;

static const char *radio_item_text(size_t index, void *userdata) {
    RadioUi *u = userdata;
    if (u->mode == MODE_COUNTRIES)
        return index < sizeof(countries) / sizeof(countries[0]) ? countries[index].name : "";
    if (u->mode == MODE_BANDS) return index < 4 ? u->band_labels[index] : "";
    if (u->mode == MODE_STATIONS && index < u->app->item_count)
        return u->catalog.stations[u->station_indices[index]].name;
    return "";
}

static void show_countries(RadioUi *u, size_t *selected) {
    catalog_cancel(&u->catalog);
    u->mode = MODE_COUNTRIES; *selected = 0;
    u->app->grid_columns = 2;
    snprintf(u->subtitle, sizeof(u->subtitle), "Selecciona un pais");
    u->app->item_count = sizeof(countries) / sizeof(countries[0]);
}

static bool ensure_backends(RadioUi *u) {
    if (!u->radio.initialized && !radio_init(&u->radio)) return false;
    if (!u->catalog.initialized && !catalog_init(&u->catalog)) return false;
    return true;
}

static void play_selected(RadioUi *u, size_t selected, char *status, size_t status_size) {
    if (selected >= u->app->item_count) return;
    size_t station = u->station_indices[selected];
    if (radio_start(&u->radio, u->catalog.stations[station].url)) {
        snprintf(u->now_playing, sizeof(u->now_playing), "%s",
                 u->catalog.stations[station].name);
        snprintf(u->now_playing_country, sizeof(u->now_playing_country), "%s",
                 countries[u->country_index].name);
        snprintf(status, status_size, "Conectando: %s", u->catalog.stations[station].name);
    } else snprintf(status, status_size, "Error: %s", radio_state(&u->radio));
}

static void show_bands(RadioUi *u, size_t *selected, char *status, size_t status_size) {
    size_t fm = 0, am = 0, other = 0;
    for (size_t i = 0; i < u->catalog.station_count; ++i) {
        if (u->catalog.stations[i].band == CATALOG_BAND_FM) ++fm;
        else if (u->catalog.stations[i].band == CATALOG_BAND_AM) ++am;
        else ++other;
    }
    snprintf(u->band_labels[0], sizeof(u->band_labels[0]), "TODAS %u", (unsigned)u->catalog.station_count);
    snprintf(u->band_labels[1], sizeof(u->band_labels[1]), "FM %u", (unsigned)fm);
    snprintf(u->band_labels[2], sizeof(u->band_labels[2]), "AM %u", (unsigned)am);
    snprintf(u->band_labels[3], sizeof(u->band_labels[3]), "OTRAS %u", (unsigned)other);
    u->mode = MODE_BANDS; u->app->grid_columns = 2; u->app->item_count = 4; *selected = 0;
    snprintf(u->subtitle, sizeof(u->subtitle), "%s: selecciona una banda", countries[u->country_index].name);
    snprintf(status, status_size, "FM/AM se identifica por el nombre publicado");
}

static void show_catalog_error(RadioUi *u, size_t *selected, char *status, size_t status_size) {
    u->mode = MODE_CATALOG_ERROR;
    u->app->grid_columns = 1;
    u->app->item_count = 0;
    *selected = 0;
    snprintf(u->subtitle, sizeof(u->subtitle), "%s: no se pudo cargar el catalogo",
             countries[u->country_index].name);
    snprintf(status, status_size, "%s | A reintentar | B volver", u->catalog.state);
}

static bool matches_filter(CatalogBand band, BandFilter filter) {
    return filter == FILTER_ALL || (filter == FILTER_FM && band == CATALOG_BAND_FM) ||
           (filter == FILTER_AM && band == CATALOG_BAND_AM) ||
           (filter == FILTER_OTHER && band == CATALOG_BAND_OTHER);
}

static void apply_filter(RadioUi *u, BandFilter filter, size_t *selected,
                         char *status, size_t status_size) {
    if (u->station_index_capacity < u->catalog.station_count) {
        size_t *indices = realloc(u->station_indices, u->catalog.station_count * sizeof(*indices));
        if (!indices) { snprintf(status, status_size, "No hay memoria para filtrar emisoras"); return; }
        u->station_indices = indices; u->station_index_capacity = u->catalog.station_count;
    }
    size_t count = 0;
    for (size_t i = 0; i < u->catalog.station_count; ++i)
        if (matches_filter(u->catalog.stations[i].band, filter)) u->station_indices[count++] = i;
    static const char *names[] = {"TODAS", "FM", "AM", "OTRAS"};
    u->mode = MODE_STATIONS; u->app->grid_columns = 1; u->app->item_count = count; *selected = 0;
    snprintf(u->subtitle, sizeof(u->subtitle), "%s - %s: %u emisoras",
             countries[u->country_index].name, names[filter], (unsigned)count);
    snprintf(status, status_size, count ? "A: reproducir | B: cambiar banda" : "No hay emisoras en esta categoria");
}

static void input(uint32_t trigger, size_t *selected, char *status,
                  size_t status_size, void *userdata) {
    RadioUi *u = userdata;
    if (trigger & VPAD_BUTTON_B) {
        if (u->mode == MODE_STATIONS) {
            show_bands(u, selected, status, status_size); return;
        }
        if (u->mode != MODE_COUNTRIES) {
            show_countries(u, selected); snprintf(status, status_size, "Selecciona un pais"); return;
        }
    }
    if (trigger & VPAD_BUTTON_X) {
        radio_stop(&u->radio);
        u->now_playing[0] = 0;
        u->now_playing_country[0] = 0;
        snprintf(status, status_size, "Reproduccion detenida");
    }
    if (trigger & VPAD_BUTTON_Y) { radio_toggle_pause(&u->radio); snprintf(status, status_size, "Estado: %s", radio_state(&u->radio)); }
    if (u->mode == MODE_STATIONS && (trigger & VPAD_BUTTON_L)) {
        if (*selected > 0) --*selected; play_selected(u, *selected, status, status_size);
    }
    if (u->mode == MODE_STATIONS && (trigger & VPAD_BUTTON_R)) {
        if (*selected + 1 < u->app->item_count) ++*selected; play_selected(u, *selected, status, status_size);
    }
    if (!(trigger & VPAD_BUTTON_A)) return;
    if (u->mode == MODE_COUNTRIES) {
        if (!ensure_backends(u)) { snprintf(status, status_size, "Error al iniciar red/audio"); return; }
        size_t country = *selected;
        if (catalog_start(&u->catalog, countries[country].code)) {
            u->country_index = country;
            u->mode = MODE_LOADING;
            u->app->grid_columns = 1;
            u->app->item_count = 0;
            snprintf(u->subtitle, sizeof(u->subtitle), "Cargando radios de %s...", countries[country].name);
            snprintf(status, status_size, "Consultando Radio Browser");
        }
    } else if (u->mode == MODE_CATALOG_ERROR) {
        if (catalog_start(&u->catalog, countries[u->country_index].code)) {
            u->mode = MODE_LOADING;
            u->app->item_count = 0;
            snprintf(u->subtitle, sizeof(u->subtitle), "Reintentando radios de %s...",
                     countries[u->country_index].name);
            snprintf(status, status_size, "Consultando Radio Browser nuevamente");
        }
    } else if (u->mode == MODE_BANDS) {
        apply_filter(u, (BandFilter)*selected, selected, status, status_size);
    } else if (u->mode == MODE_STATIONS) play_selected(u, *selected, status, status_size);
}

static void tick(size_t *selected, char *status, size_t status_size, void *userdata) {
    RadioUi *u = userdata;
    if (u->radio.initialized && u->radio.running) {
        radio_update(&u->radio);
        snprintf(status, status_size, "Estado: %s", radio_state(&u->radio));
    }
    if (u->mode == MODE_LOADING) {
        catalog_update(&u->catalog);
        snprintf(status, status_size, "%s", u->catalog.state);
        if (u->catalog.ready) {
            if (u->catalog.station_count) show_bands(u, selected, status, status_size);
            else show_catalog_error(u, selected, status, status_size);
        }
    }
}

static void shutdown_all(void *userdata) {
    RadioUi *u = userdata;
    catalog_shutdown(&u->catalog); radio_shutdown(&u->radio);
    free(u->station_indices); u->station_indices = NULL; u->station_index_capacity = 0;
}

int main(void) {
    ToribioApp app = {
        .title = "Toribio Radio Internacional",
        .subtitle = ui.subtitle,
        .footer = "A Play | Y Pausa | X Stop | L/R Cambiar | B Volver",
        .now_playing_title = ui.now_playing,
        .now_playing_country = ui.now_playing_country,
        .item_text = radio_item_text,
        .on_input = input, .on_tick = tick, .on_shutdown = shutdown_all,
        .userdata = &ui,
        .tv_background_jpeg = radio_bg_tv_jpg,
        .tv_background_size = radio_bg_tv_jpg_size,
        .drc_background_jpeg = radio_bg_drc_jpg,
        .drc_background_size = radio_bg_drc_jpg_size,
        .tv_watermark_rgba = toribio_watermark_tv_rgba,
        .tv_watermark_size = toribio_watermark_tv_rgba_size,
        .tv_watermark_width = 300, .tv_watermark_height = 87,
        .drc_watermark_rgba = toribio_watermark_drc_rgba,
        .drc_watermark_size = toribio_watermark_drc_rgba_size,
        .drc_watermark_width = 200, .drc_watermark_height = 58
    };
    ui.app = &app;
    size_t selected = 0; show_countries(&ui, &selected);
    return toribio_run(&app);
}
