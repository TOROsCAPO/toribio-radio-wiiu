#include "catalog_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const catalog_servers[] = {
    "https://de2.api.radio-browser.info",
    "https://radios.axiomaudio.com"
};
#define CATALOG_SERVER_COUNT (sizeof(catalog_servers) / sizeof(catalog_servers[0]))

static size_t receive_catalog(char *ptr, size_t size, size_t nmemb, void *userdata) {
    CatalogBackend *c = userdata;
    size_t bytes = size * nmemb;
    size_t available = CATALOG_BUFFER_SIZE - 1 - c->data_size;
    if (bytes > available) {
        c->data_truncated = true;
        return 0;
    }
    memcpy(c->data + c->data_size, ptr, bytes);
    c->data_size += bytes;
    c->data[c->data_size] = 0;
    return bytes;
}

static void trim(char *text) {
    size_t n = strlen(text);
    while (n && (text[n - 1] == '\r' || text[n - 1] == '\n' || text[n - 1] == ' ')) text[--n] = 0;
}

static char latin_base(unsigned char value) {
    switch (value) {
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: return 'A';
        case 0x87: return 'C';
        case 0x88: case 0x89: case 0x8A: case 0x8B: return 'E';
        case 0x8C: case 0x8D: case 0x8E: case 0x8F: return 'I';
        case 0x91: return 'N';
        case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: return 'O';
        case 0x99: case 0x9A: case 0x9B: case 0x9C: return 'U';
        case 0x9D: return 'Y';
        case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: return 'a';
        case 0xA7: return 'c';
        case 0xA8: case 0xA9: case 0xAA: case 0xAB: return 'e';
        case 0xAC: case 0xAD: case 0xAE: case 0xAF: return 'i';
        case 0xB1: return 'n';
        case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: return 'o';
        case 0xB9: case 0xBA: case 0xBB: case 0xBC: return 'u';
        case 0xBD: case 0xBF: return 'y';
        default: return 0;
    }
}

static void make_display_name(char *target, size_t target_size, const char *source) {
    size_t out = 0;
    const unsigned char *p = (const unsigned char *)source;
    while (*p && out + 1 < target_size) {
        char value = 0;
        if (*p >= 32 && *p < 127) {
            value = (char)*p++;
        } else if (*p == 0xC3 && p[1]) {
            value = latin_base(p[1]); p += 2;
        } else if (*p == '\t') {
            value = ' '; ++p;
        } else {
            unsigned char first = *p++;
            int extra = (first & 0xE0) == 0xC0 ? 1 : (first & 0xF0) == 0xE0 ? 2 :
                        (first & 0xF8) == 0xF0 ? 3 : 0;
            while (extra-- > 0 && (*p & 0xC0) == 0x80) ++p;
        }
        if (value == ' ' && (out == 0 || target[out - 1] == ' ')) continue;
        if (value) target[out++] = value;
    }
    while (out && target[out - 1] == ' ') --out;
    target[out] = 0;
    if (!out) snprintf(target, target_size, "Emisora sin nombre");
}

static bool station_is_duplicate(const CatalogBackend *c, const char *url) {
    for (size_t i = 0; i < c->station_count; ++i)
        if (!strcmp(c->stations[i].url, url)) return true;
    return false;
}

static bool is_http_url(const char *text) {
    return !strncmp(text, "http://", 7) || !strncmp(text, "https://", 8);
}

static bool is_valid_m3u(const CatalogBackend *c) {
    const unsigned char *p = (const unsigned char *)c->data;
    if (c->data_size >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) p += 3;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
    return !strncmp((const char *)p, "#EXTM3U", 7);
}

static bool has_band_word(const char *text, char first, char second) {
    for (size_t i = 0; text[i] && text[i + 1]; ++i) {
        char a = text[i], b = text[i + 1];
        if (a >= 'a' && a <= 'z') a -= 'a' - 'A';
        if (b >= 'a' && b <= 'z') b -= 'a' - 'A';
        char previous = i ? text[i - 1] : 0, next = text[i + 2];
        bool previous_letter = (previous >= 'A' && previous <= 'Z') || (previous >= 'a' && previous <= 'z');
        bool next_letter = (next >= 'A' && next <= 'Z') || (next >= 'a' && next <= 'z');
        if (a == first && b == second && !previous_letter && !next_letter) return true;
    }
    return false;
}

static CatalogBand detect_band(const char *name) {
    if (has_band_word(name, 'F', 'M') || strstr(name, "MHz") || strstr(name, "MHZ"))
        return CATALOG_BAND_FM;
    if (has_band_word(name, 'A', 'M') || strstr(name, "kHz") || strstr(name, "KHZ"))
        return CATALOG_BAND_AM;
    for (const char *p = name; *p; ++p) {
        if (*p < '0' || *p > '9') continue;
        char *end = NULL;
        double frequency = strtod(p, &end);
        if (end > p && (*end == '.' || strchr(p, '.')) && frequency >= 87.0 && frequency <= 108.9)
            return CATALOG_BAND_FM;
        if (end > p) p = end - 1;
    }
    return CATALOG_BAND_OTHER;
}

static bool reserve_station(CatalogBackend *c) {
    if (c->station_count < c->station_capacity) return true;
    size_t capacity = c->station_capacity ? c->station_capacity * 2 : 512;
    CatalogStation *stations = realloc(c->stations, capacity * sizeof(*stations));
    if (!stations) { c->memory_full = true; return false; }
    c->stations = stations;
    c->station_capacity = capacity;
    return true;
}

static size_t parse_m3u_page(CatalogBackend *c, size_t *received) {
    size_t before = c->station_count;
    *received = 0;
    char pending_name[72] = "Emisora sin nombre";
    bool has_station_info = false;
    char *cursor = c->data;
    while (*cursor) {
        char *end = strchr(cursor, '\n');
        if (end) *end = 0;
        trim(cursor);
        if (!strncmp(cursor, "#EXTINF", 7)) {
            char *comma = strchr(cursor, ',');
            if (comma && comma[1]) {
                make_display_name(pending_name, sizeof(pending_name), comma + 1);
                has_station_info = true;
            }
        } else if (*cursor && *cursor != '#' && has_station_info && is_http_url(cursor)) {
            ++*received;
            if (!station_is_duplicate(c, cursor)) {
                if (!reserve_station(c)) break;
                CatalogStation *station = &c->stations[c->station_count++];
                snprintf(station->name, sizeof(station->name), "%s", pending_name);
                snprintf(station->url, sizeof(station->url), "%s", cursor);
                station->band = detect_band(station->name);
            }
            snprintf(pending_name, sizeof(pending_name), "Emisora sin nombre");
            has_station_info = false;
        }
        if (!end) break;
        cursor = end + 1;
    }
    return c->station_count - before;
}

bool catalog_init(CatalogBackend *c) {
    memset(c, 0, sizeof(*c));
    c->multi = curl_multi_init();
    c->initialized = c->multi != NULL;
    return c->initialized;
}

static bool start_page(CatalogBackend *c) {
    if (!c->multi) {
        c->multi = curl_multi_init();
        if (!c->multi) return false;
    }
    char url[512];
    snprintf(url, sizeof(url),
             "%s/m3u/stations/search?countrycode=%s&codec=MP3&hidebroken=true&order=clickcount&reverse=true&offset=%u&limit=%u",
             catalog_servers[c->server_index], c->country_code,
             (unsigned)c->offset, (unsigned)CATALOG_PAGE_SIZE);
    c->easy = curl_easy_init();
    if (!c->easy) return false;
    c->data_size = 0;
    c->data[0] = 0;
    c->data_truncated = false;
    c->http_status = 0;
    curl_easy_setopt(c->easy, CURLOPT_URL, url);
    curl_easy_setopt(c->easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c->easy, CURLOPT_USERAGENT, "Toribio-WiiU-Radio/0.18.0-beta.1");
    curl_easy_setopt(c->easy, CURLOPT_CONNECTTIMEOUT_MS, 7000L);
    curl_easy_setopt(c->easy, CURLOPT_TIMEOUT_MS, 20000L);
    curl_easy_setopt(c->easy, CURLOPT_LOW_SPEED_LIMIT, 128L);
    curl_easy_setopt(c->easy, CURLOPT_LOW_SPEED_TIME, 10L);
    curl_easy_setopt(c->easy, CURLOPT_CAINFO, "/vol/content/ca-bundle.crt");
    curl_easy_setopt(c->easy, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(c->easy, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(c->easy, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(c->easy, CURLOPT_FRESH_CONNECT, 1L);
    curl_easy_setopt(c->easy, CURLOPT_FORBID_REUSE, 1L);
    curl_easy_setopt(c->easy, CURLOPT_WRITEFUNCTION, receive_catalog);
    curl_easy_setopt(c->easy, CURLOPT_WRITEDATA, c);
    if (curl_multi_add_handle(c->multi, c->easy) != CURLM_OK) {
        curl_easy_cleanup(c->easy); c->easy = NULL; return false;
    }
    c->running = true;
    snprintf(c->state, sizeof(c->state), "Cargando emisoras: %u (servidor %u/%u)...",
             (unsigned)c->station_count, c->server_index + 1,
             (unsigned)CATALOG_SERVER_COUNT);
    return true;
}

bool catalog_start(CatalogBackend *c, const char *country_code) {
    if (!c->initialized) return false;
    catalog_cancel(c);
    /* Una sesion nueva evita mensajes CURLMSG_DONE viejos al cambiar de pais. */
    if (c->multi) curl_multi_cleanup(c->multi);
    c->multi = curl_multi_init();
    if (!c->multi) { snprintf(c->state, sizeof(c->state), "No se pudo reiniciar la red"); return false; }
    c->station_count = 0; c->offset = 0; c->ready = false; c->memory_full = false;
    c->retry_count = 0; c->retry_wait_frames = 0; c->server_index = 0;
    snprintf(c->country_code, sizeof(c->country_code), "%.2s", country_code);
    return start_page(c);
}

void catalog_update(CatalogBackend *c) {
    if (!c->running) {
        if (!c->retry_wait_frames) return;
        if (--c->retry_wait_frames) return;
        if (!start_page(c)) {
            c->ready = true;
            snprintf(c->state, sizeof(c->state), "No se pudo reintentar el catalogo");
        }
        return;
    }
    int active = 0;
    CURLMcode result = curl_multi_perform(c->multi, &active);
    if (result != CURLM_OK) {
        snprintf(c->state, sizeof(c->state), "Catalogo: %s", curl_multi_strerror(result));
        c->running = false; c->ready = true; return;
    }
    int remaining = 0; CURLMsg *message;
    bool load_next = false;
    while ((message = curl_multi_info_read(c->multi, &remaining))) {
        if (message->msg == CURLMSG_DONE && message->easy_handle == c->easy) {
            CURLcode done = message->data.result;
            curl_easy_getinfo(c->easy, CURLINFO_RESPONSE_CODE, &c->http_status);
            curl_multi_remove_handle(c->multi, c->easy);
            curl_easy_cleanup(c->easy); c->easy = NULL;
            bool valid_response = done == CURLE_OK && c->http_status >= 200 &&
                                  c->http_status < 300 && !c->data_truncated &&
                                  is_valid_m3u(c);
            if (valid_response) {
                c->retry_count = 0;
                size_t received = 0;
                parse_m3u_page(c, &received);
                if (!c->memory_full && received == CATALOG_PAGE_SIZE) {
                    c->offset += CATALOG_PAGE_SIZE;
                    load_next = true;
                } else {
                    c->ready = true;
                    snprintf(c->state, sizeof(c->state), c->memory_full ?
                             "Memoria completa: %u emisoras" : "%u emisoras MP3 encontradas",
                             (unsigned)c->station_count);
                }
            } else if (c->server_index + 1 < CATALOG_SERVER_COUNT) {
                ++c->retry_count;
                ++c->server_index;
                c->station_count = 0;
                c->offset = 0;
                c->memory_full = false;
                c->retry_wait_frames = 90;
                snprintf(c->state, sizeof(c->state), "Servidor no disponible; probando alternativa %u/%u...",
                         c->server_index + 1, (unsigned)CATALOG_SERVER_COUNT);
            } else {
                c->ready = true;
                if (c->station_count)
                    snprintf(c->state, sizeof(c->state), "Carga parcial: %u emisoras",
                             (unsigned)c->station_count);
                else if (done != CURLE_OK)
                    snprintf(c->state, sizeof(c->state), "Catalogo: %s", curl_easy_strerror(done));
                else if (c->http_status >= 400)
                    snprintf(c->state, sizeof(c->state), "Servidor de radios: HTTP %ld",
                             c->http_status);
                else if (c->data_truncated)
                    snprintf(c->state, sizeof(c->state), "La respuesta del catalogo fue demasiado grande");
                else if (!is_valid_m3u(c))
                    snprintf(c->state, sizeof(c->state), "El servidor devolvio un catalogo invalido");
                else
                    snprintf(c->state, sizeof(c->state), "No se pudo cargar el catalogo");
            }
            c->running = false;
        }
    }
    if (load_next && !start_page(c)) {
        c->ready = true;
        snprintf(c->state, sizeof(c->state), "Carga parcial: %u emisoras", (unsigned)c->station_count);
    }
}

void catalog_cancel(CatalogBackend *c) {
    c->running = false; c->ready = false; c->retry_wait_frames = 0;
    if (c->easy) {
        curl_multi_remove_handle(c->multi, c->easy);
        curl_easy_cleanup(c->easy); c->easy = NULL;
    }
    if (c->multi) {
        int remaining = 0;
        while (curl_multi_info_read(c->multi, &remaining)) { }
    }
}

void catalog_shutdown(CatalogBackend *c) {
    if (!c->initialized) return;
    catalog_cancel(c);
    curl_multi_cleanup(c->multi); c->multi = NULL;
    free(c->stations); c->stations = NULL; c->station_capacity = c->station_count = 0;
    c->initialized = false;
}
