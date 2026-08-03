#include "radio_backend.h"
#include "tls_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    RadioBackend *r = userdata;
    memset(stream, 0, (size_t)len);
    SDL_LockMutex(r->mutex);
    if (r->paused || r->buffering) {
        SDL_UnlockMutex(r->mutex);
        return;
    }
    size_t wanted = (size_t)len;
    if (wanted > r->used) {
        wanted = r->used;
        r->underrun = true;
    }
    size_t first = RADIO_RING_SIZE - r->read_pos;
    if (first > wanted) first = wanted;
    memcpy(stream, r->ring + r->read_pos, first);
    memcpy(stream + first, r->ring, wanted - first);
    r->read_pos = (r->read_pos + wanted) % RADIO_RING_SIZE;
    r->used -= wanted;
    SDL_UnlockMutex(r->mutex);
}

static void ring_write(RadioBackend *r, const unsigned char *data, size_t len) {
    SDL_LockMutex(r->mutex);
    if (len > RADIO_RING_SIZE) {
        data += len - RADIO_RING_SIZE;
        len = RADIO_RING_SIZE;
    }
    size_t free_space = RADIO_RING_SIZE - r->used;
    if (len > free_space) {
        size_t drop = len - free_space;
        r->read_pos = (r->read_pos + drop) % RADIO_RING_SIZE;
        r->used -= drop;
        ++r->overflow_count;
    }
    size_t first = RADIO_RING_SIZE - r->write_pos;
    if (first > len) first = len;
    memcpy(r->ring + r->write_pos, data, first);
    memcpy(r->ring, data + first, len - first);
    r->write_pos = (r->write_pos + len) % RADIO_RING_SIZE;
    r->used += len;
    SDL_UnlockMutex(r->mutex);
}

static size_t receive_audio(char *ptr, size_t size, size_t nmemb, void *userdata) {
    RadioBackend *r = userdata;
    size_t bytes = size * nmemb;
    if (!r->running || !r->decoder) return 0;
    mpg123_feed(r->decoder, (const unsigned char *)ptr, bytes);
    unsigned char pcm[32768];
    for (int iteration = 0; iteration < 64; ++iteration) {
        size_t done = 0;
        int result = mpg123_read(r->decoder, pcm, sizeof(pcm), &done);
        if (done) ring_write(r, pcm, done);
        if (result == MPG123_NEED_MORE || result == MPG123_DONE) break;
        if (result != MPG123_OK && result != MPG123_NEW_FORMAT) break;
    }
    return bytes;
}

bool radio_init(RadioBackend *r) {
    memset(r, 0, sizeof(*r));
    r->ring = malloc(RADIO_RING_SIZE);
    if (!r->ring) return false;
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) goto fail_ring;
    if (mpg123_init() != MPG123_OK) goto fail_sdl;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) goto fail_mpg123;
    r->mutex = SDL_CreateMutex();
    r->multi = curl_multi_init();
    SDL_AudioSpec wanted = {0};
    wanted.freq = 48000;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 2;
    wanted.samples = 2048;
    wanted.callback = audio_callback;
    wanted.userdata = r;
    r->device = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
    if (!r->mutex || !r->multi || !r->device) goto fail_all;
    SDL_PauseAudioDevice(r->device, 1);
    r->initialized = true;
    snprintf(r->state, sizeof(r->state), "Listo para conectar");
    return true;

fail_all:
    if (r->device) SDL_CloseAudioDevice(r->device);
    if (r->multi) curl_multi_cleanup(r->multi);
    if (r->mutex) SDL_DestroyMutex(r->mutex);
    curl_global_cleanup();
fail_mpg123:
    mpg123_exit();
fail_sdl:
    SDL_Quit();
fail_ring:
    free(r->ring);
    memset(r, 0, sizeof(*r));
    return false;
}

bool radio_start(RadioBackend *r, const char *url) {
    if (!r->initialized) return false;
    radio_stop(r);
    /* No reutilizar el multi anterior: en Wii U real puede retener mensajes DONE. */
    if (r->multi) curl_multi_cleanup(r->multi);
    r->multi = curl_multi_init();
    if (!r->multi) {
        snprintf(r->state, sizeof(r->state), "No se pudo reiniciar la red");
        return false;
    }
    int error = MPG123_OK;
    r->decoder = mpg123_new(NULL, &error);
    r->easy = curl_easy_init();
    if (!r->decoder || !r->easy) {
        snprintf(r->state, sizeof(r->state), "No se pudo iniciar red/MP3");
        radio_stop(r);
        return false;
    }
    mpg123_param(r->decoder, MPG123_FORCE_RATE, 48000, 0.0);
    mpg123_format_none(r->decoder);
    mpg123_format(r->decoder, 48000, MPG123_STEREO, MPG123_ENC_SIGNED_16);
    mpg123_open_feed(r->decoder);
    SDL_LockMutex(r->mutex);
    r->read_pos = r->write_pos = r->used = 0;
    r->underrun = false;
    r->underrun_count = 0;
    r->overflow_count = 0;
    SDL_UnlockMutex(r->mutex);
    curl_easy_setopt(r->easy, CURLOPT_URL, url);
    curl_easy_setopt(r->easy, CURLOPT_FOLLOWLOCATION, 1L);
    r->curl_error[0] = 0;
    curl_easy_setopt(r->easy, CURLOPT_ERRORBUFFER, r->curl_error);
    if (!toribio_configure_tls(r->easy)) {
        snprintf(r->state, sizeof(r->state), "No se pudo configurar HTTPS");
        radio_stop(r);
        return false;
    }
    curl_easy_setopt(r->easy, CURLOPT_USERAGENT, "Toribio-WiiU-Radio/0.18.0-beta.2");
    curl_easy_setopt(r->easy, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(r->easy, CURLOPT_DNS_CACHE_TIMEOUT, 60L);
    curl_easy_setopt(r->easy, CURLOPT_CONNECTTIMEOUT_MS, 5000L);
    curl_easy_setopt(r->easy, CURLOPT_LOW_SPEED_LIMIT, 128L);
    curl_easy_setopt(r->easy, CURLOPT_LOW_SPEED_TIME, 15L);
    curl_easy_setopt(r->easy, CURLOPT_BUFFERSIZE, 16384L);
    curl_easy_setopt(r->easy, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(r->easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(r->easy, CURLOPT_WRITEFUNCTION, receive_audio);
    curl_easy_setopt(r->easy, CURLOPT_WRITEDATA, r);
    if (curl_multi_add_handle(r->multi, r->easy) != CURLM_OK) {
        snprintf(r->state, sizeof(r->state), "No se pudo agregar conexion");
        radio_stop(r);
        return false;
    }
    r->running = true;
    r->paused = false;
    r->buffering = true;
    SDL_PauseAudioDevice(r->device, 1);
    snprintf(r->state, sizeof(r->state), "Cargando buffer: 0%%");
    return true;
}

void radio_update(RadioBackend *r) {
    if (!r->initialized || !r->running || !r->multi) return;
    int active = 0;
    CURLMcode result = curl_multi_perform(r->multi, &active);
    if (result != CURLM_OK) {
        snprintf(r->state, sizeof(r->state), "Multi: %s", curl_multi_strerror(result));
        radio_stop(r);
        return;
    }
    int messages = 0;
    CURLMsg *message;
    while ((message = curl_multi_info_read(r->multi, &messages))) {
        if (message->msg == CURLMSG_DONE && message->easy_handle == r->easy) {
            if (message->data.result != CURLE_OK)
                snprintf(r->state, sizeof(r->state), "Red e%u: %.72s",
                         (unsigned)message->data.result,
                         r->curl_error[0] ? r->curl_error : curl_easy_strerror(message->data.result));
            else
                snprintf(r->state, sizeof(r->state), "Stream finalizado");
            r->running = false;
        }
    }
    if (!r->running) return;

    SDL_LockMutex(r->mutex);
    size_t buffered = r->used;
    bool ran_dry = r->underrun;
    r->underrun = false;
    SDL_UnlockMutex(r->mutex);

    if (!r->paused && !r->buffering &&
        (ran_dry || buffered < RADIO_LOW_WATER_SIZE)) {
        r->buffering = true;
        ++r->underrun_count;
        SDL_PauseAudioDevice(r->device, 1);
    }

    if (r->buffering) {
        if (buffered >= RADIO_PREBUFFER_SIZE) {
            r->buffering = false;
            if (!r->paused) SDL_PauseAudioDevice(r->device, 0);
            snprintf(r->state, sizeof(r->state), "Reproduciendo MP3 (buffer 4 s)");
        } else {
            unsigned percent = (unsigned)((buffered * 100u) / RADIO_PREBUFFER_SIZE);
            snprintf(r->state, sizeof(r->state), r->underrun_count
                         ? "Recargando buffer: %u%%"
                         : "Cargando buffer: %u%%", percent);
        }
    } else if (!r->paused) {
        snprintf(r->state, sizeof(r->state), "Reproduciendo MP3 (buffer %u s)",
                 (unsigned)(buffered / RADIO_PCM_BYTES_PER_SECOND));
    }
}

void radio_stop(RadioBackend *r) {
    r->running = false;
    r->paused = false;
    r->buffering = false;
    if (r->device) SDL_PauseAudioDevice(r->device, 1);
    if (r->easy) {
        if (r->multi) curl_multi_remove_handle(r->multi, r->easy);
        curl_easy_cleanup(r->easy);
        r->easy = NULL;
    }
    if (r->multi) {
        int remaining = 0;
        while (curl_multi_info_read(r->multi, &remaining)) { }
    }
    if (r->decoder) {
        mpg123_close(r->decoder);
        mpg123_delete(r->decoder);
        r->decoder = NULL;
    }
    if (r->mutex) {
        SDL_LockMutex(r->mutex);
        r->read_pos = r->write_pos = r->used = 0;
        r->underrun = false;
        SDL_UnlockMutex(r->mutex);
    }
}

void radio_shutdown(RadioBackend *r) {
    if (!r->initialized) return;
    radio_stop(r);
    if (r->multi) curl_multi_cleanup(r->multi);
    if (r->device) SDL_CloseAudioDevice(r->device);
    if (r->mutex) SDL_DestroyMutex(r->mutex);
    curl_global_cleanup();
    mpg123_exit();
    SDL_Quit();
    free(r->ring);
    r->ring = NULL;
    r->initialized = false;
}

const char *radio_state(RadioBackend *r) { return r->state; }

void radio_toggle_pause(RadioBackend *r) {
    if (!r->initialized || !r->running) return;
    r->paused = !r->paused;
    SDL_PauseAudioDevice(r->device, (r->paused || r->buffering) ? 1 : 0);
    snprintf(r->state, sizeof(r->state), r->paused ? "Pausado" :
             (r->buffering ? "Cargando buffer..." : "Reproduciendo MP3"));
}
