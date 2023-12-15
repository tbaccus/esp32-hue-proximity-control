#include "esp_http_client.h"

#include "hue_https.h"

static const char* tag = "hue_https";

typedef enum {
    HUE_DATA_TYPE_LIGHT = 0,
    HUE_DATA_TYPE_GROUPED_LIGHT,
    HUE_DATA_TYPE_SMART_SCENE,
    HUE_DATA_TYPE_SCENE,
} hue_data_type_t;

typedef struct {
    hue_data_type_t data_type : 2;
    union {
        hue_light_data_t* light;
        hue_grouped_light_data_t* grouped_light;
        hue_smart_scene_data_t* smart_scene;
    };
} hue_data_t;

typedef char s_hue_request_buffer_t[HUE_REQUEST_BUFFER_SIZE];

static esp_http_client_handle_t hue_create_https_client(s_hue_request_buffer_t request_buffer, hue_config_t* hue_config);



extern const char hue_signify_root_cert_pem_start[] asm("_binary_hue_signify_root_cert_pem_start");
extern const char hue_signify_root_cert_pem_end[] asm("_binary_hue_signify_root_cert_pem_end");
