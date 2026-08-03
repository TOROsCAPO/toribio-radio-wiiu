#pragma once
#include <curl/curl.h>
#include <stdbool.h>
#include <stddef.h>

#define CATALOG_BUFFER_SIZE (384 * 1024)
#define CATALOG_PAGE_SIZE 250

typedef enum { CATALOG_BAND_OTHER, CATALOG_BAND_FM, CATALOG_BAND_AM } CatalogBand;
typedef struct { char name[72]; char url[384]; CatalogBand band; } CatalogStation;
typedef struct {
    CURLM *multi;
    CURL *easy;
    bool initialized, running, ready;
    char data[CATALOG_BUFFER_SIZE];
    size_t data_size;
    CatalogStation *stations;
    size_t station_count, station_capacity;
    size_t offset;
    char country_code[3];
    bool memory_full;
    bool data_truncated;
    unsigned retry_count;
    unsigned retry_wait_frames;
    unsigned server_index;
    long http_status;
    char state[96];
} CatalogBackend;

bool catalog_init(CatalogBackend *catalog);
bool catalog_start(CatalogBackend *catalog, const char *country_code);
void catalog_update(CatalogBackend *catalog);
void catalog_cancel(CatalogBackend *catalog);
void catalog_shutdown(CatalogBackend *catalog);
