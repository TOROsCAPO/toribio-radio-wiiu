#pragma once
#include <SDL2/SDL.h>
#include <curl/curl.h>
#include <mpg123.h>
#include <stdbool.h>
#include <stddef.h>

#define RADIO_PCM_BYTES_PER_SECOND (48000 * 2 * 2)
#define RADIO_RING_SIZE (2 * 1024 * 1024)
#define RADIO_PREBUFFER_SIZE (4 * RADIO_PCM_BYTES_PER_SECOND)
#define RADIO_LOW_WATER_SIZE (RADIO_PCM_BYTES_PER_SECOND)
typedef struct {
    bool initialized;
    bool running;
    bool paused;
    bool buffering;
    bool underrun;
    SDL_AudioDeviceID device;
    SDL_mutex *mutex;
    CURLM *multi;
    CURL *easy;
    mpg123_handle *decoder;
    unsigned char *ring;
    size_t read_pos, write_pos, used;
    unsigned underrun_count;
    unsigned overflow_count;
    char state[96];
} RadioBackend;

bool radio_init(RadioBackend *radio);
bool radio_start(RadioBackend *radio, const char *url);
void radio_update(RadioBackend *radio);
void radio_stop(RadioBackend *radio);
void radio_shutdown(RadioBackend *radio);
const char *radio_state(RadioBackend *radio);
void radio_toggle_pause(RadioBackend *radio);
