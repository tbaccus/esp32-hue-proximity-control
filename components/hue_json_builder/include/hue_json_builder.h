/**
 * @file hue_json_builder.h
 * @author Tanner Baccus
 * @date 09 December 2023
 * @brief Declaration of all public functions for the generation of HTTP request JSON bodies from Hue data structures
 */

#ifndef H_HUE_JSON_BUILDER
#define H_HUE_JSON_BUILDER

#include "esp_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*====================================================================================================================*/
/*===================================================== Defines ======================================================*/
/*====================================================================================================================*/

#define HUE_JSON_BUFFER_SIZE 256 /**< Maximum number of characters for JSON buffer */

#define HUE_RESOURCE_TYPE_SIZE 14 /**< Length of "grouped_light/", the longest supported resource identifier */
#define HUE_RESOURCE_TYPE_MIN 6   /**< Length of "light/", the shortest supported resource identifier */

/* Brightness setting bounds */
#define HUE_MIN_B_SET 1   /**< Minimum value for brightness setting */
#define HUE_MAX_B_SET 100 /**< Maximum value for brightness setting */
#define HUE_MIN_B_ADD 0   /**< Minimum value for brightness modifying */
#define HUE_MAX_B_ADD 100 /**< Maximum value for brightness modifying */

/* Color temp setting bounds */
#define HUE_MIN_CT_SET 153 /**< Minimum value for color temp setting */
#define HUE_MAX_CT_SET 500 /**< Maximum value for color temp setting */
#define HUE_MIN_CT_ADD 0   /**< Minimum value for color temp modifying */
#define HUE_MAX_CT_ADD 347 /**< Maximum value for color temp modifying */

/*====================================================================================================================*/
/*=========================================== Public Structure Definitions ===========================================*/
/*====================================================================================================================*/

/** @brief Buffers for JSON string creation for set buffer sizes */
typedef struct {
    const char* resource_type;       /**< Buffer for resource type string */
    const char* resource_id;         /**< Buffer for Hue resource ID */
    char buff[HUE_JSON_BUFFER_SIZE]; /**< Buffer with set size */
} hue_json_buffer_t;

/** @brief Enum for how a value in a hue json structure should be interpreted */
typedef enum {
    HUE_ACTION_NONE = 0, /**< Don't take any action */
    HUE_ACTION_SET,      /**< Set hue's value */
    HUE_ACTION_ADD,      /**< Add to hue's value */
    HUE_ACTION_SUBTRACT  /**< Subtract from hue's value */
} hue_action_t;

/** @brief Settings for Philips Hue light resources */
typedef struct {
    const char* resource_id; /**< Hue resource ID */
    bool off : 1;                           /**< Light off (true) or on (false) */
    hue_action_t brightness_action : 2;     /**< How brightness should be adjusted */
    uint8_t brightness : 7;                 /**< [0-100] Amount brightness should be adjusted by or set to */
    hue_action_t color_temp_action : 2;     /**< How color temp should be adjusted */
    uint16_t color_temp : 9;                /**< Amount color temp should be adjusted by [0-347] or set to [153-500] */
    bool set_color : 1;                     /**< If color_gamut values should be used */
    uint16_t color_gamut_x : 14;            /**< CIE X gamut position decimal value (e.g. 123 = 0.0123, >=10000 = 1) */
    uint16_t color_gamut_y : 14;            /**< CIE Y gamut position decimal value (e.g. 123 = 0.0123, >=10000 = 1) */
} hue_light_data_t;

/** @brief Settings for Philips Hue light group resources */
typedef hue_light_data_t hue_grouped_light_data_t;

/** @brief Settings for Philips Hue smart scene resources */
typedef struct {
    const char* resource_id; /**< Hue resource ID */
    bool deactivate : 1;                    /**< Dectivate (true) or activate (false) smart scene */
} hue_smart_scene_data_t;

// TODO: Implement hue_scene_data_t structure
/** @brief Settings for Philips Hue scene resources */
/* typedef struct {
    const char* resource_id;
} hue_scene_data_t; */

/*====================================================================================================================*/
/*=========================================== Public Function Declarations ===========================================*/
/*====================================================================================================================*/

/**
 * @brief Converts hue_light_data structure into JSON for HTTP request
 *
 * @param[out] json_buffer Buffer to store output string, custom type to enforce buffer size
 * @param[in] hue_data Hue API JSON tags as data structure
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Buffer successfully filled with JSON conversion
 * @retval - @c ESP_ERR_INVALID_ARG – json_buffer, hue_data, or their internal buffers are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Encoding failure during buffer writing
 * @retval - @c ESP_ERR_INVALID_SIZE – Buffer is too small for JSON output
 *
 * @note This function will clip values out of range for Hue's API
 */
esp_err_t hue_light_data_to_json(hue_json_buffer_t* json_buffer, hue_light_data_t* hue_data);

/**
 * @brief Converts hue_grouped_light_data structure into JSON for HTTP request
 *
 * @param[out] json_buffer Buffer to store output string, custom type to enforce buffer size
 * @param[in] hue_data Hue API JSON tags as data structure
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Buffer successfully filled with JSON conversion
 * @retval - @c ESP_ERR_INVALID_ARG – json_buffer, hue_data, or their internal buffers are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Encoding failure during buffer writing
 * @retval - @c ESP_ERR_INVALID_SIZE – Buffer is too small for JSON output
 *
 * @note This function will clip values out of range for Hue's API
 */
esp_err_t hue_grouped_light_data_to_json(hue_json_buffer_t* json_buffer, hue_grouped_light_data_t* hue_data);

/**
 * @brief Converts hue_smart_scene_data structure into JSON for HTTP request
 *
 * @param[out] json_buffer Buffer to store output string, custom type to enforce buffer size
 * @param[in] hue_data Hue API JSON tags as data structure
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Buffer successfully filled with JSON conversion
 * @retval - @c ESP_ERR_INVALID_ARG – json_buffer, hue_data, or their internal buffers are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Encoding failure during buffer writing
 * @retval - @c ESP_ERR_INVALID_SIZE – Buffer is too small for JSON output
 */
esp_err_t hue_smart_scene_data_to_json(hue_json_buffer_t* json_buffer, hue_smart_scene_data_t* hue_data);

#ifdef __cplusplus
}
#endif
#endif /* H_HUE_JSON_BUILDER */