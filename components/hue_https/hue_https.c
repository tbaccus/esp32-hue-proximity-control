#include "esp_http_client.h"

#include "hue_https.h"

static const char* tag = "hue_https";

extern const char hue_signify_root_cert_pem_start[] asm("_binary_hue_signify_root_cert_pem_start");
extern const char hue_signify_root_cert_pem_end[] asm("_binary_hue_signify_root_cert_pem_end");


typedef enum {
    HUE_RESOURCE_TYPE_LIGHT = 0,
    HUE_RESOURCE_TYPE_GROUPED_LIGHT,
    HUE_RESOURCE_TYPE_SMART_SCENE,
    HUE_RESOURCE_TYPE_SCENE,
} hue_resource_type_t;

/** @brief Buffer for request response with set size */
typedef char s_hue_request_buffer_t[HUE_REQUEST_BUFFER_SIZE];

/**
 * @brief Perform HTTPS request using Hue config and resource data
 *
 * @param[in] hue_config Pointer to Hue config
 * @param[in] json_buffer Pointer to JSON buffer filled with output from hue_json_builder function
 * @param[in] resource_id ID of resource to perform actions on
 * @param[in] resource_type Type of Hue resource being used
 *
 * @return ESP Error Code
 * @retval - @c ESP_OK – Successfully performed action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – hue_config or json_buffer are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code from bridge did not equal 200 OK
 * @retval - @c ESP_ERR_INVALID_SIZE – Request buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
static esp_err_t hue_https_request(hue_config_t* hue_config, hue_json_buffer_t* json_buffer, const char* resource_id,
                                   hue_resource_type_t resource_type);

/**
 * @brief Perform HTTPS request using Hue config and resource data
 *
 * @param[in] hue_config Pointer to Hue config
 * @param[in] json_buffer Pointer to JSON buffer filled with output from hue_json_builder function
 * @param[in] resource_id ID of resource to perform actions on
 * @param[in] resource_type Type of Hue resource being used
 *
 * @return ESP Error Code
 * @retval - @c ESP_OK – Successfully performed action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – hue_config or json_buffer are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code from bridge did not equal 200 OK
 * @retval - @c ESP_ERR_INVALID_SIZE – Request buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
static esp_err_t hue_https_request(hue_config_t* hue_config, hue_json_buffer_t* json_buffer, const char* resource_id,
                                   hue_resource_type_t resource_type) {
    char url_str[] = "https://000.000.000.000/clip/v2/resource/aaaaaaaaaaaaa/ffffffff-ffff-ffff-ffff-ffffffffffff";
    snprintf(url_str, strlen(url_str), "https://%d.%d.%d.%d/clip/v2/resource/", hue_config->bridge_ip4_address[0],
             hue_config->bridge_ip4_address[1], hue_config->bridge_ip4_address[2], hue_config->bridge_ip4_address[3]);
    switch(resource_type) {
        case HUE_RESOURCE_TYPE_LIGHT:
            strcat(url_str, "light/");
            break;
        case HUE_RESOURCE_TYPE_GROUPED_LIGHT:
            strcat(url_str, "grouped_light/");
            break;
        case HUE_RESOURCE_TYPE_SMART_SCENE:
            strcat(url_str, "smart_scene/");
            break;
        case HUE_RESOURCE_TYPE_SCENE:
            strcat(url_str, "scene/");
            break;
    }
    strncat(url_str, resource_id, strlen("ffffffff-ffff-ffff-ffff-ffffffffffff"));

    s_hue_request_buffer_t request_buffer;
    esp_http_client_config_t config = {.url = url_str,
                                       .cert_pem = hue_signify_root_cert_pem_start,
                                       .common_name = hue_config->bridge_id,
                                       .user_data = request_buffer,
                                       /* TODO: .event_handler = event_handler, */
                                       .timeout_ms = 5000,
                                       .method = HTTP_METHOD_PUT};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "hue-application-key", hue_config->application_key);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    /* TDOD: esp_http_client_set_post_field */
    /* TDOD: esp_http_client_perform */
    
    esp_http_client_cleanup(client);
}