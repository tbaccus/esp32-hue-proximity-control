#include <string.h>
#include <stdarg.h>

#include "esp_log.h"

#include "hue_helpers.h"
#include "hue_json_builder.h"

static const char* tag = "hue_json_builder";

/* TODO: Hue Scene Resource JSON construction (more testing of request needed to implement properly)
 *  [ ] scene
 *       [ ] recall:{action: str[active, static],
 *                   duration: uint,
 *                   dimming:{brightness: int[1-100]}}
 */

/**
 * @brief Clamps value to inclusive range and sends warning
 *
 * @param[in] value Value to be clamped
 * @param[in] minimum Minimum allowed value
 * @param[in] maximum Maximum allowed value
 *
 * @return Clamped value
 */
static uint16_t hue_clamp(uint16_t value, uint16_t minimum, uint16_t maximum) {
    if (value > maximum) {
        ESP_LOGW(tag, "%d too large, clamped to %d", value, maximum);
        return maximum;
    }
    if (value < minimum) {
        ESP_LOGW(tag, "%d too small, clamped to %d", value, minimum);
        return minimum;
    }
    return value;
}

/**
 * @brief Append printf formatted string to JSON buffer and verify its success
 *
 * @param[out] json_buffer Buffer to append output to
 * @param[in] format Format string in printf style
 * @param[in] args... Format string arguments
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – String appended successfully
 * @retval - @c ESP_ERR_INVALID_ARG – json_buffer or underlying buffer is NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – snprintf returned encoding failure
 * @retval - @c ESP_ERR_INVALID_SIZE – Buffer string too short to append to
 */
static esp_err_t hue_json_sprintf_and_check(hue_json_buffer_t* json_buffer, const char* format, ...) {
    if (HUE_NULL_CHECK(tag, json_buffer)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer->buff)) return ESP_ERR_INVALID_ARG;

    /* Position used for appending to buffer */
    int buff_pos = strlen(json_buffer->buff);

    /* Maximum number of characters allowed to append to prevent buffer overflow */
    const uint16_t chars_left = HUE_JSON_BUFFER_SIZE - buff_pos;

    /* Pass format and arguments into vsnprintf */
    va_list args;
    va_start(args, format);
    buff_pos = vsnprintf(json_buffer->buff + buff_pos, chars_left, format, args);
    va_end(args);

    if (buff_pos < 0) {
        ESP_LOGE(tag, "JSON string printing encoding failure");
        return ESP_ERR_INVALID_RESPONSE;
    }
    if (buff_pos >= chars_left) {
        ESP_LOGE(tag, "JSON string ran out of characters to print to");
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

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
esp_err_t hue_light_data_to_json(hue_json_buffer_t* json_buffer, hue_light_data_t* hue_data) {
    if (HUE_NULL_CHECK(tag, json_buffer)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer->buff)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer->resource_id)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer->resource_type)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, hue_data)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, hue_data->resource_id)) return ESP_ERR_INVALID_ARG;

    /* Pass resource type and ID to json_buffer */
    strcpy(json_buffer->resource_type, "light");
    strcpy(json_buffer->resource_id, hue_data->resource_id);

    esp_err_t ret = ESP_OK;                             /* Error code variable for print error checking */
    memset(json_buffer->buff, 0, HUE_JSON_BUFFER_SIZE); /* Clear output buffer, as printing function appends */

    /* Prints "on" tag and checks if successful */
    ret = hue_json_sprintf_and_check(json_buffer, "{\"on\":{\"on\":%s}", HUE_BOOL_STR(!(hue_data->off)));
    if (ret != ESP_OK) return ret; /* Return if any error found during printing */

    /* Prints "dimming" or "dimming_delta" tags with value clamping and checks if successful */
    switch (hue_data->brightness_action) {
        case (HUE_ACTION_SET): /* Use "dimming" tag if setting brightness */
            ret = hue_json_sprintf_and_check(json_buffer, ",\"dimming\":{\"brightness\":%d}",
                                             hue_clamp(hue_data->brightness, HUE_MIN_B_SET, HUE_MAX_B_SET));
            break;
        case (HUE_ACTION_ADD): /* Use "dimming_delta" tag if adding to brightness */
            ret = hue_json_sprintf_and_check(json_buffer,
                                             ",\"dimming_delta\":{\"action\":\"up\",\"brightness_delta\":%d}",
                                             hue_clamp(hue_data->brightness, HUE_MIN_B_ADD, HUE_MAX_B_ADD));
            break;
        case (HUE_ACTION_SUBTRACT): /* Use "dimming_delta" tag if subtracting from brightness */
            ret = hue_json_sprintf_and_check(json_buffer,
                                             ",\"dimming_delta\":{\"action\":\"down\",\"brightness_delta\":%d}",
                                             hue_clamp(hue_data->brightness, HUE_MIN_B_ADD, HUE_MAX_B_ADD));
            break;
        default: /* If any other value, ignore "dimming" and "dimming_delta" tags entirely */
    };
    if (ret != ESP_OK) return ret; /* Return if any error found during printing */

    /* Prints "color_temperature" or "color_temperature_delta" tags with value clamping and checks if successful */
    switch (hue_data->color_temp_action) {
        case (HUE_ACTION_SET): /* Use "color_temperature" tag if setting color temperature */
            ret = hue_json_sprintf_and_check(json_buffer, ",\"color_temperature\":{\"mirek\":%d}",
                                             hue_clamp(hue_data->color_temp, HUE_MIN_CT_SET, HUE_MAX_CT_SET));
            break;
        case (HUE_ACTION_ADD): /* Use "color_temperature_delta" tag if adding to color temperature */
            ret = hue_json_sprintf_and_check(json_buffer,
                                             ",\"color_temperature_delta\":{\"action\":\"up\",\"mirek_delta\":%d}",
                                             hue_clamp(hue_data->color_temp, HUE_MIN_CT_ADD, HUE_MAX_CT_ADD));
            break;
        case (HUE_ACTION_SUBTRACT): /* Use "color_temperature_delta" tag if subtracting from color temperature */
            ret = hue_json_sprintf_and_check(json_buffer,
                                             ",\"color_temperature_delta\":{\"action\":\"down\",\"mirek_delta\":%d}",
                                             hue_clamp(hue_data->color_temp, HUE_MIN_CT_ADD, HUE_MAX_CT_ADD));
            break;
        default: /* If any other value, ignore "color_temperature" and "color_temperature_delta" tags entirely */
    };
    if (ret != ESP_OK) return ret; /* Return if any error found during printing */

    /* Prints "color" tag if enabled and checks if successful */
    if (hue_data->set_color) {
        /* Buffers for int conversion into decimal without use of floats, any value over 10000 clamped to "1.0" */
        char x_buff[7] = "1.0";
        if (hue_data->color_gamut_x < 10000) snprintf(x_buff, sizeof(x_buff), "0.%04d", hue_data->color_gamut_x);
        char y_buff[7] = "1.0";
        if (hue_data->color_gamut_y < 10000) snprintf(y_buff, sizeof(y_buff), "0.%04d", hue_data->color_gamut_y);

        /* Print converted values to "color:xy" tag */
        ret = hue_json_sprintf_and_check(json_buffer, ",\"color\":{\"xy\":{\"x\":%s,\"y\":%s}}", x_buff, y_buff);
        if (ret != ESP_OK) return ret; /* Return if any error found during printing */
    }

    /* Print closing bracket and return final error code, ESP_OK will return only if no issues have occurred anywhere */
    return hue_json_sprintf_and_check(json_buffer, "}");
}

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
esp_err_t hue_grouped_light_data_to_json(hue_json_buffer_t* json_buffer, hue_grouped_light_data_t* hue_data) {
    /* Grouped lights and light resources currently use the same tags, so no need for separate function */
    esp_err_t err = hue_light_data_to_json(json_buffer, hue_data);

    /* Pass resource type and ID to json_buffer */
    strcpy(json_buffer->resource_type, "grouped_light");
    strcpy(json_buffer->resource_id, hue_data->resource_id);
    return err;
} 

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
esp_err_t hue_smart_scene_data_to_json(hue_json_buffer_t* json_buffer, hue_smart_scene_data_t* hue_data) {
    if (HUE_NULL_CHECK(tag, json_buffer)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer->buff)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer->resource_id)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, hue_data)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, hue_data->resource_id)) return ESP_ERR_INVALID_ARG;
    
    /* Pass resource type and ID to json_buffer */
    strcpy(json_buffer->resource_type, "smart_scene");
    strcpy(json_buffer->resource_id, hue_data->resource_id);

    memset(json_buffer->buff, 0, HUE_JSON_BUFFER_SIZE); /* Clear output buffer, as printing function appends */

    /* Prints "recall" tag and returns success/failure code */
    return hue_json_sprintf_and_check(json_buffer, "{\"recall\":{\"action\":%s}}",
                                      hue_data->deactivate ? "\"deactivate\"" : "\"activate\"");
}