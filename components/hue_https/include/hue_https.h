/**
 * @file hue_https.h
 * @author Tanner Baccus
 * @date 13 December 2023
 * @brief Declarations for all public functions used for creating and performing Philips Hue HTTPS requests
 */

#ifndef H_HUE_HTTPS
#define H_HUE_HTTPS

#include "esp_types.h"
#include "esp_err.h"
#include "esp_bit_defs.h"

#include "hue_json_builder.h"

#ifdef __cplusplus
extern "C" {
#endif

/*====================================================================================================================*/
/*=========================================== Public Structure Definitions ===========================================*/
/*====================================================================================================================*/

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
    const char* const task_id; /**< ID to assign to Hue HTTPS instance task */
    uint8_t retry_attempts;    /**< Maximum number of times to retry HTTPS request before failing */
} hue_https_config_t;

typedef struct hue_https_instance* hue_https_handle_t;                 /**< Handle for hue_https session */
typedef struct hue_https_request_instance* hue_https_request_handle_t; /**< Handle for created request */

/*====================================================================================================================*/
/*=========================================== Public Function Declarations ===========================================*/
/*====================================================================================================================*/

/**
 * @brief Create HTTPS request instance using hue light data for actions to perform
 *
 * @param[out] p_request_handle Request handle to store instance into to be used with hue https instance
 * @param[in] p_light_data Actions to be performed on specified light resource
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Request instance successfully created
 * @retval - @c ESP_ERR_INVALID_ARG – p_request_handle, p_light_data, or p_light_data's resource ID are NULL or resource
 * ID is not in the correct format as specified by the Philips Hue API
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Encoding error encountered during JSON generation
 * @retval - @c ESP_ERR_INVALID_SIZE – JSON buffer was too small or a failure occurred with copying of data to request
 * instance
 * @retval - @c ESP_ERR_NO_MEM – Failed to allocate memory for request instance
 */
esp_err_t hue_https_create_light_request(hue_https_request_handle_t* p_request_handle, hue_light_data_t* p_light_data);

/**
 * @brief Create HTTPS request instance using hue grouped light data for actions to perform
 *
 * @param[out] p_request_handle Request handle to store instance into to be used with hue https instance
 * @param[in] p_grouped_light_data Actions to be performed on specified grouped light resource
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Request instance successfully created
 * @retval - @c ESP_ERR_INVALID_ARG – p_request_handle, p_grouped_light_data, or p_grouped_light_data's resource ID are
 * NULL or resource ID is not in the correct format as specified by the Philips Hue API
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Encoding error encountered during JSON generation
 * @retval - @c ESP_ERR_INVALID_SIZE – JSON buffer was too small or a failure occurred with copying of data to request
 * instance
 * @retval - @c ESP_ERR_NO_MEM – Failed to allocate memory for request instance
 */
esp_err_t hue_https_create_grouped_light_request(hue_https_request_handle_t* p_request_handle,
                                                 hue_grouped_light_data_t* p_grouped_light_data);

/**
 * @brief Create HTTPS request instance using hue smart scene data for actions to perform
 *
 * @param[out] p_request_handle Request handle to store instance into to be used with hue https instance
 * @param[in] p_smart_scene_data Actions to be performed on specified smart scene resource
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Request instance successfully created
 * @retval - @c ESP_ERR_INVALID_ARG – p_request_handle, p_smart_scene_data, or p_smart_scene_data's resource ID are NULL
 * or resource ID is not in the correct format as specified by the Philips Hue API
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Encoding error encountered during JSON generation
 * @retval - @c ESP_ERR_INVALID_SIZE – JSON buffer was too small or a failure occurred with copying of data to request
 * instance
 * @retval - @c ESP_ERR_NO_MEM – Failed to allocate memory for request instance
 */
esp_err_t hue_https_create_smart_scene_request(hue_https_request_handle_t* p_request_handle,
                                               hue_smart_scene_data_t* p_smart_scene_data);

/**
 * @brief Destroys HTTPS request instance and frees all associated resources
 *
 * @param[in,out] p_request_handle Pointer to request handle to destroy (Will be set to NULL after success)
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Request instance successfully destroyed and freed
 */
esp_err_t hue_https_destroy_request(hue_https_request_handle_t* p_request_handle);

/**
 * @brief Creates a Hue HTTPS instance for sending Hue HTTPS requests
 *
 * @param[out] p_hue_https_handle Hue HTTPS handle to store instance into
 * @param[in] p_hue_https_config Hue HTTPS configuration data
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Hue HTTPS instance successfully allocated and defined
 * @retval - @c ESP_ERR_INVALID_ARG – p_hue_https_handle, p_hue_https_config, or p_hue_https_config's internal pointers
 * are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Encoding error encountered with Bridge URL base sprintf call
 * @retval - @c ESP_ERR_INVALID_SIZE – Bridge IP, ID, or Application Key in p_hue_https_config are not valid
 * @retval - @c ESP_ERR_NO_MEM – Failed to allocate memory or to create Event Group, Mutex, or Task for Hue HTTPS
 * instance
 */
esp_err_t hue_https_create_instance(hue_https_handle_t* p_hue_https_handle, hue_https_config_t* p_hue_https_config);

/**
 * @brief Destroys Hue HTTPS instance and frees all associated resources
 *
 * @param[in,out] p_hue_https_handle Pointer to Hue HTTPS handle to destroy (Will be set to NULL after success)
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Hue HTTPS instance successfully destroyed and freed
 */
esp_err_t hue_https_destroy_instance(hue_https_handle_t* p_hue_https_handle);

#ifdef __cplusplus
}
#endif
#endif /* H_HUE_HTTPS */