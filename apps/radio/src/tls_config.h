#pragma once

#include <stdbool.h>
#include <curl/curl.h>

/* Compatibilidad de libcurl/mbedTLS en Wii U: cifra, pero no valida el certificado. */
bool toribio_configure_tls(CURL *easy);
