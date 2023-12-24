#ifndef H_HUE_HTTPS
#define H_HUE_HTTPS

#include "esp_types.h"
#include "esp_err.h"
#include "esp_bit_defs.h"

#include "hue_json_builder.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HUE_REQUEST_BUFFER_SIZE 512

/** Bridge ID scanf format with 16 hexadecimal characters */
#define HUE_BRIDGE_ID_FORMAT "%*16lx"
/** Length of bridge ID without null-terminating character */
#define HUE_BRIDGE_ID_LENGTH 16

/** Bridge IP scanf format with standard 4 byte IP formatting */
#define HUE_BRIDGE_IP_FORMAT "%*3hhu.%*3hhu.%*3hhu.%*3hhu"
/** Length of bridge IP without null-terminating character */
#define HUE_BRIDGE_IP_LENGTH 15

/** Application key scanf format with 40 URL Base64 characters */
#define HUE_APPLICATION_KEY_FORMAT "%*40[-_0-9a-zA-Z]"
/** Length of application key without null-terminating character */
#define HUE_APPLICATION_KEY_LENGTH 40

/** Resource ID scanf format using hexadecimal characters: "[8 chars]-[4 chars]-[4 chars]-[4 chars]-[12 chars]" */
#define HUE_RESOURCE_ID_FORMAT "%*8lx-%*4lx-%*4lx-%*4lx-%*12lx"
/** Length of resource ID format without null-terminating character */
#define HUE_RESOURCE_ID_LENGTH 36

#define HUE_RESOURCE_PATH "/clip/v2/resource/" /**< Philips Hue path to resource */
#define HUE_RESOURCE_PATH_LENGTH 18            /**< Length of HUE_RESOURCE_PATH without null-terminating character */

/** Size of "https://" + IPV4 address + HUE_RESOURCE_PATH + longest resource type id + resource id length */
#define HUE_URL_BUFFER_SIZE 8 + 15 + HUE_RESOURCE_PATH_LENGTH + HUE_RESOURCE_TYPE_SIZE + HUE_RESOURCE_ID_LENGTH + 1

#define HUE_HTTPS_EVT_WIFI_CONNECTED_BIT BIT0

/**
 * @brief Philips Hue bridge information and application key for requests
 *
 * @attention Information must be aquired while on the same network as the bridge
 */
typedef struct {
    const char* bridge_ip; /**< IP address of bridge from https://discovery.meethue.com */
    const char* bridge_id; /**< Bridge ID from https://discovery.meethue.com */

    /** Application key obtained by following API tutorial on
     * https://developers.meethue.com/develop/hue-api-v2/getting-started/ */
    const char* application_key;
    uint8_t retry_attempts; /**< Maximum number of times to retry HTTPS request before failing */
} hue_https_config_t;

typedef struct hue_https_instance* hue_https_handle_t;                 /**< Handle for hue_https session */
typedef struct hue_https_request_instance* hue_https_request_handle_t; /**< Handle for created request */

/**
 * @brief Sends HTTPS request to configured Philips Hue bridge for controlling a light resource
 *
 * @param[in] p_light_data Pointer to structure with actions to perform on resource
 * @param[in] p_hue_config Pointer to structure with Philips Hue bridge information and application key for requests
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Successfully performed light resource action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – p_light_data or p_hue_config are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code from bridge did not equal 200 OK
 * @retval - @c ESP_ERR_INVALID_SIZE – Request buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
esp_err_t hue_light_https_request(hue_light_data_t* p_light_data);

/**
 * @brief Sends HTTPS request to configured Philips Hue bridge for controlling a grouped light resource
 *
 * @param[in] p_grouped_light_data Pointer to structure with actions to perform on resource
 * @param[in] p_hue_config Pointer to structure with Philips Hue bridge information and application key for requests
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Successfully performed grouped light resource action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – p_grouped_light_data or p_hue_config are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code from bridge did not equal 200 OK
 * @retval - @c ESP_ERR_INVALID_SIZE – Request buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
esp_err_t hue_grouped_light_https_request(hue_grouped_light_data_t* p_grouped_light_data);

/**
 * @brief Sends HTTPS request to configured Philips Hue bridge for controlling a smart scene resource
 *
 * @param[in] p_smart_scene_data Pointer to structure with actions to perform on resource
 * @param[in] p_hue_config Pointer to structure with Philips Hue bridge information and application key for requests
 * @param[in] timeout Max time to wait for hue_https_report_wifi_connected() to be called before exiting (0 for max
 * delay)
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Successfully performed smart scene resource action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – p_smart_scene_data or p_hue_config are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code not 200 OK or encoding error encountered
 * @retval - @c ESP_ERR_INVALID_SIZE – Request or URL buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
esp_err_t hue_smart_scene_https_request(hue_smart_scene_data_t* p_smart_scene_data, uint32_t timeout);

void hue_https_report_wifi_connected();
void hue_https_report_wifi_disconnected();

#ifdef __cplusplus
}
#endif

#endif /* H_HUE_HTTPS */