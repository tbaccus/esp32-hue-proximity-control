#include <string.h>
#include <stdarg.h>
#include "esp_err.h"
#include "esp_log.h"
#include "hue_helpers.h"
#include "hue_json_builder.h"

static const char* tag = "hue_json_builder";

/* TODO: Hue Resource JSON construction
 *  [ ] light
 *       [x] on:{on: bool}
 *       [x] dimming:{brightness: int[1-100]}
 *       [x] dimming_delta:{action: str[up, down, stop],
 *                          brightness_delta: int[0-100]}
 *       [x] color_temperature:{mirek: int[153-500]}
 *       [x] color_temperature_delta:{action: str[up, down, stop],
 *                                    mirek_delta: int[0-347]}
 *       [x] color:{xy:{x: float[0-1],
 *                      y: float[0-1]}}
 *  [ ] scene
 *       [ ] recall:{action: str[active, static],
 *                   duration: uint,
 *                   dimming:{brightness: int[1-100]}}
 *  [ ] grouped_light
 *       [x] on:{on: bool}
 *       [x] dimming:{brightness: int[1-100]}
 *       [x] dimming_delta:{action: str[up, down, stop],
 *                          brightness_delta: int[0-100]}
 *       [x] color_temperature:{mirek: int[153-500]}
 *       [x] color_temperature_delta:{action: str[up, down, stop],
 *                                    mirek_delta: int[0-347]}
 *       [x] color:{xy:{x: float[0-1],
 *                      y: float[0-1]}}
 *  [ ] smart_scene
 *       [x] recall:{action: str[activate, deactivate]}
 */

/**
 * @brief Validates value is within bounds
 *
 * @param[in] value_string String name of value
 * @param[in] value Value to check
 * @param[in] min_value Minimum value (inclusive)
 * @param[in] max_value Maximum value (inclusive)
 *
 * @return ESP error code
 * @retval - @c ESP_OK – Value passed bound checks
 * @retval - @c ESP_ERR_INVALID_ARG – Value out of bounds
 */
// static esp_err_t hue_data_value_check(const char* value_string, uint32_t value, uint32_t min_value, uint32_t
// max_value) {
//     if((value < min_value) || (value > max_value)) {
//         ESP_LOGE(tag, "%s value (%d) out of range [%d-%d]", value_string, value, min_value, max_value);
//         return ESP_ERR_INVALID_ARG;
//     }
//     return ESP_OK;
// }

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
    if (!HUE_NULL_CHECK(tag, json_buffer)) return ESP_ERR_INVALID_ARG;
    if (!HUE_NULL_CHECK(tag, json_buffer->buff)) return ESP_ERR_INVALID_ARG;
    int buff_pos = strlen(json_buffer->buff);
    const uint16_t chars_left = HUE_JSON_BUFFER_SIZE - buff_pos;

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

static char* hue_gamut_to_str(char* buffer, uint16_t value) {
    if (value > 9999)
        snprintf(buffer, strlen("0.0000") + 1, "1");
    else
        snprintf(buffer, strlen("0.0000") + 1, "0.%04d", value);
    return buffer;
}

static esp_err_t hue_light_data_to_json(hue_json_buffer_t* json_buffer, hue_light_data_t* hue_data) {
    if (!HUE_NULL_CHECK(tag, json_buffer)) return ESP_ERR_INVALID_ARG;
    if (!HUE_NULL_CHECK(tag, json_buffer->buff)) return ESP_ERR_INVALID_ARG;
    if (!HUE_NULL_CHECK(tag, hue_data)) return ESP_ERR_INVALID_ARG;
    esp_err_t ret = ESP_OK;
    memset(json_buffer->buff, 0, HUE_JSON_BUFFER_SIZE);

    /* Prints "on" tag and checks if successful */
    ret = hue_json_sprintf_and_check(json_buffer, "{\"on\":{\"on\":%s}", HUE_BOOL_STR(hue_data->on));
    if (ret != ESP_OK) return ret;

    switch (hue_data->brightness_action) {
        case (HUE_ACTION_SET):
            ret = hue_json_sprintf_and_check(json_buffer, ",\"dimming\":{\"brightness\":%d}", hue_data->brightness);
            break;
        case (HUE_ACTION_ADD):
            ret = hue_json_sprintf_and_check(
                json_buffer, ",\"dimming_delta\":{\"action\":\"up\",\"brightness_delta\":%d}", hue_data->brightness);
            break;
        case (HUE_ACTION_SUBTRACT):
            ret = hue_json_sprintf_and_check(
                json_buffer, ",\"dimming_delta\":{\"action\":\"down\",\"brightness_delta\":%d}", hue_data->brightness);
            break;
        default:
    };
    if (ret != ESP_OK) return ret;

    switch (hue_data->color_temp_action) {
        case (HUE_ACTION_SET):
            ret =
                hue_json_sprintf_and_check(json_buffer, ",\"color_temperature\":{\"mirek\":%d}", hue_data->color_temp);
            break;
        case (HUE_ACTION_ADD):
            ret = hue_json_sprintf_and_check(json_buffer,
                                             ",\"color_temperature_delta\":{\"action\":\"up\",\"mirek_delta\":%d}",
                                             hue_data->color_temp);
            break;
        case (HUE_ACTION_SUBTRACT):
            ret = hue_json_sprintf_and_check(json_buffer,
                                             ",\"color_temperature_delta\":{\"action\":\"down\",\"mirek_delta\":%d}",
                                             hue_data->color_temp);
            break;
        default:
    };
    if (ret != ESP_OK) return ret;

    if (hue_data->set_color) {
        char x_buff[] = "0.0000";
        char y_buff[] = "0.0000";
        ret = hue_json_sprintf_and_check(json_buffer, ",\"color\":{\"xy\":{\"x\":%s,\"y\":%s}}",
                                         hue_gamut_to_str(x_buff, hue_data->color_gamut_x),
                                         hue_gamut_to_str(y_buff, hue_data->color_gamut_y));
        if (ret != ESP_OK) return ret;
    }

    return hue_json_sprintf_and_check(json_buffer, "}");
}