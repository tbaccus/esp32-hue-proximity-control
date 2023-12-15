#include "esp_http_client.h"

#include "hue_https.h"

static const char* tag = "hue_https";

typedef char s_hue_request_buffer_t[HUE_REQUEST_BUFFER_SIZE];

static esp_http_client_handle_t hue_create_https_client(s_hue_request_buffer_t request_buffer,
                                                        hue_config_t* hue_config);

extern const char hue_signify_root_cert_pem_start[] asm("_binary_hue_signify_root_cert_pem_start");
extern const char hue_signify_root_cert_pem_end[] asm("_binary_hue_signify_root_cert_pem_end");
