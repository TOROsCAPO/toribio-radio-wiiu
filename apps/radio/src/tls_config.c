#include "tls_config.h"

bool toribio_configure_tls(CURL *easy) {
    if (!easy) return false;
    return curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L) == CURLE_OK &&
           curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L) == CURLE_OK;
}
