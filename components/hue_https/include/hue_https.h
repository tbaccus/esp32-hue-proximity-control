#ifndef H_HUE_HTTPS
#define H_HUE_HTTPS

#include "esp_types.h"
#include "esp_err.h"

#include "hue_json_builder.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HUE_REQUEST_BUFFER_SIZE 512

#define HUE_RESOURCE_PATH "/clip/v2/resource/" /**< Philips Hue path to resource */
#define HUE_RESOURCE_PATH_SIZE 18              /**< Length of HUE_RESOURCE_PATH without null terminator */

/** Size of "https://" + IPV4 address + HUE_RESOURCE_PATH + longest resource type id + resource id length */
#define HUE_URL_BUFFER_SIZE 8 + 15 + HUE_RESOURCE_PATH_SIZE + HUE_RESOURCE_TYPE_SIZE + HUE_RESOURCE_ID_SIZE

/**
 * @brief Philips Hue bridge information and application key for requests
 *
 * @attention Information must be aquired while on the same network as the bridge
 */
typedef struct {
    uint8_t bridge_ip[4];  /**< IP address of bridge from https://discovery.meethue.com */
    const char* bridge_id; /**< Bridge ID from https://discovery.meethue.com */

    /** Application key obtained by following API tutorial on
     * https://developers.meethue.com/develop/hue-api-v2/getting-started/ */
    const char* application_key;
    uint8_t retry_attempts; /**< Maximum number of times to retry HTTPS request before failing */
} hue_config_t;

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
esp_err_t hue_light_https_request(hue_light_data_t* p_light_data, hue_config_t* p_hue_config);

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
esp_err_t hue_grouped_light_https_request(hue_grouped_light_data_t* p_grouped_light_data, hue_config_t* p_hue_config);

/**
 * @brief Sends HTTPS request to configured Philips Hue bridge for controlling a smart scene resource
 *
 * @param[in] p_smart_scene_data Pointer to structure with actions to perform on resource
 * @param[in] p_hue_config Pointer to structure with Philips Hue bridge information and application key for requests
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Successfully performed smart scene resource action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – p_smart_scene_data or p_hue_config are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code from bridge did not equal 200 OK
 * @retval - @c ESP_ERR_INVALID_SIZE – Request buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
esp_err_t hue_smart_scene_https_request(hue_smart_scene_data_t* p_smart_scene_data, hue_config_t* p_hue_config);

#ifdef __cplusplus
}
#endif

#endif /* H_HUE_HTTPS */