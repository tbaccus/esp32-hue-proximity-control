#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_log.h"

#include "hue_https.h"
#include "hue_helpers.h"

static const char* tag = "hue_https";

extern const char hue_signify_root_cert_pem_start[] asm("_binary_hue_signify_root_cert_pem_start");
extern const char hue_signify_root_cert_pem_end[] asm("_binary_hue_signify_root_cert_pem_end");

/** @brief Buffer for request response with set size */
typedef char s_hue_request_buffer_t[HUE_REQUEST_BUFFER_SIZE];

/**
 * @brief Converts HTTP Event ID enum into string representation
 *
 * @param[in] id HTTP Event ID
 *
 * @return Human-readable string representation of HTTP Event ID
 */
static const char* http_event_id_to_str(esp_http_client_event_id_t id);

/**
 * @brief Perform HTTPS request using Hue config and resource data
 *
 * @param[in] hue_config Pointer to Hue config
 * @param[in] json_buffer Pointer to JSON buffer filled with output from hue_json_builder function
 *
 * @return ESP Error Code
 * @retval - @c ESP_OK – Successfully performed action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – hue_config, json_buffer, or underlying buffers are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code not 200 OK or encoding error encountered
 * @retval - @c ESP_ERR_INVALID_SIZE – Request or URL buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
static esp_err_t hue_https_request(hue_config_t* hue_config, hue_json_buffer_t* json_buffer);

/**
 * @brief Event handler for HTTP session, as defined by esp_http_client
 *
 * @param[in] evt Event pointer
 *
 * @return ESP Error code, only returns ESP_OK, more research needed as to the purpose of this return
 */
static esp_err_t hue_https_event_handler(esp_http_client_event_t* evt);

/**
 * @brief Converts HTTP Event ID enum into string representation
 *
 * @param[in] id HTTP Event ID
 *
 * @return Human-readable string representation of HTTP Event ID
 */
static const char* http_event_id_to_str(esp_http_client_event_id_t id) {
    switch (id) {
        case HTTP_EVENT_ERROR:
            return "HTTP_EVENT_ERROR";
        case HTTP_EVENT_ON_CONNECTED:
            return "HTTP_EVENT_ON_CONNECTED";
        case HTTP_EVENT_HEADERS_SENT:
            return "HTTP_EVENT_HEADERS_SENT";
        case HTTP_EVENT_ON_HEADER:
            return "HTTP_EVENT_ON_HEADER";
        case HTTP_EVENT_ON_DATA:
            return "HTTP_EVENT_ON_DATA";
        case HTTP_EVENT_ON_FINISH:
            return "HTTP_EVENT_ON_FINISH";
        case HTTP_EVENT_DISCONNECTED:
            return "HTTP_EVENT_DISCONNECTED";
        case HTTP_EVENT_REDIRECT:
            return "HTTP_EVENT_REDIRECT";
        default:
            return "HTTP_EVENT...";
    }
}

/**
 * @brief Perform HTTPS request using Hue config and resource data
 *
 * @param[in] hue_config Pointer to Hue config
 * @param[in] json_buffer Pointer to JSON buffer filled with output from hue_json_builder function
 *
 * @return ESP Error Code
 * @retval - @c ESP_OK – Successfully performed action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – hue_config, json_buffer, or underlying buffers are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code not 200 OK or encoding error encountered
 * @retval - @c ESP_ERR_INVALID_SIZE – Request or URL buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
static esp_err_t hue_https_request(hue_config_t* hue_config, hue_json_buffer_t* json_buffer) {
    if (HUE_NULL_CHECK(tag, hue_config)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, hue_config->bridge_id)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, hue_config->application_key)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer->buff)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer->resource_id)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer->resource_type)) return ESP_ERR_INVALID_ARG;

    /* Create URL for bridge resource using set IP and resource information */
    char url_str[HUE_URL_BUFFER_SIZE];
    esp_err_t err = snprintf(url_str, HUE_URL_BUFFER_SIZE, "https://%d.%d.%d.%d" HUE_RESOURCE_PATH "%s/%s",
                             hue_config->bridge_ip[0], hue_config->bridge_ip[1], hue_config->bridge_ip[2],
                             hue_config->bridge_ip[3], json_buffer->resource_type, json_buffer->resource_id);

    ESP_LOGD(tag, "Generated URL: [%s]", url_str);

    /* Verify that URL has been created before moving on */
    if (err <= 0) {
        ESP_LOGE(tag, "Hue request URL generation encountered encoding error");
        return ESP_ERR_INVALID_RESPONSE;
    } else if (err >= HUE_URL_BUFFER_SIZE) {
        ESP_LOGE(tag, "Hue request URL too long for buffer (actual %d >= max %d)", err, HUE_URL_BUFFER_SIZE);
        return ESP_ERR_INVALID_SIZE;
    }

    /* Create and initialize http client handle */
    s_hue_request_buffer_t request_buffer;
    esp_http_client_config_t config = {.url = url_str,
                                       .cert_pem = hue_signify_root_cert_pem_start, /* CA cert used for TLS */
                                       .common_name = hue_config->bridge_id,        /* CN used to verify CA cert */
                                       .user_data = request_buffer,                 /* Buffer to store respones */
                                       .event_handler = hue_https_event_handler,    /* Event handler function */
                                       .timeout_ms = 5000,                          /* Halved to speed up on fail */
                                       .method = HTTP_METHOD_PUT};
    esp_http_client_handle_t client = NULL; /* Declare variable for retry handling */

    err = ESP_FAIL;          /* For request error checking and retrying */
    uint8_t attempt_num = 0; /* Counter for attempts for retry_attempts configuration */

    /* Retry until request successfully delivers or retry_attempts configuration is reached */
    while (attempt_num <= hue_config->retry_attempts) {
        ESP_LOGD(tag, "Request attempt #%d", attempt_num + 1);

        /* Determined retry handling works best when completely destroying connection on failure */
        client = esp_http_client_init(&config);

        if (!client) {
            ESP_LOGE(tag, "Client handle failed to be created");
            return ESP_ERR_INVALID_RESPONSE;
        }

        /* Set headers used by Hue API */
        esp_http_client_set_header(client, "hue-application-key", hue_config->application_key);
        esp_http_client_set_header(client, "Content-Type", "application/json");

        /* Add generated actions to request */
        esp_http_client_set_post_field(client, json_buffer->buff, strlen(json_buffer->buff));

        if ((err = esp_http_client_perform(client)) == ESP_OK) {
            /* If response status code is not 200 OK, log actual status and set for return ESP_ERR_INVALID_RESPONSE */
            if (esp_http_client_get_status_code(client) != HttpStatus_Ok) {
                ESP_LOGE(tag, "HTTP response status not 200 OK, recieved %d", esp_http_client_get_status_code(client));
                ESP_LOGD(tag, "HTTP response:\n\t%s", request_buffer);
                err = ESP_ERR_INVALID_RESPONSE;
            }

            /* Clean up connection and return ESP_OK or ESP_ERR_INVALID_RESPONSE */
            esp_http_client_cleanup(client); 
            return err;
        }

        /* Clean up client for retry with less lingering errors */
        esp_http_client_cleanup(client);

        /* Tracking for number of attempts */
        attempt_num++;
        ESP_LOGD(tag, "Error performing HTTP request on attempt #%d: %s", attempt_num, esp_err_to_name(err));

        /* Wait 1 second before retrying HTTP request */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* Only reached if max number of attempts is reached, report error */
    ESP_LOGE(tag, "HTTPS request failed after %d attempts", attempt_num);

    return ESP_FAIL;
}

/**
 * @brief Event handler for HTTP session, as defined by esp_http_client
 *
 * @param[in] evt Event pointer
 *
 * @return ESP Error code, only returns ESP_OK, more research needed as to the purpose of this return
 */
static esp_err_t hue_https_event_handler(esp_http_client_event_t* evt) {
    static size_t chars_output = 0; /* Number of characters output to buffer */

    switch (evt->event_id) {
        case HTTP_EVENT_ON_HEADER: /* Header recieved from server */
            ESP_LOGD(tag, "HTTP Event HTTP_EVENT_ON_HEADER, %s: %s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA: /* Data recieved from server */
            ESP_LOGD(tag, "HTTP Event HTTP_EVENT_ON_DATA\n\tData length = %d\n\t%.*s", evt->data_len, evt->data_len - 2,
                     (char*)evt->data);
            if (!evt->user_data) return ESP_OK; /* If output buffer doesn't exist, exit */

            /* If no data has been output, clear output buffer */
            if (chars_output == 0) memset(evt->user_data, 0, HUE_REQUEST_BUFFER_SIZE);

            /* Append output data to buffer, ensuring data cannot overflow the buffer */
            if ((HUE_REQUEST_BUFFER_SIZE - 1 - chars_output) > 0) {
                strncat(evt->user_data, evt->data, HUE_REQUEST_BUFFER_SIZE - 1 - chars_output);
                chars_output = strlen(evt->user_data); /* Update the number of characters output */
            }
            break;
        case HTTP_EVENT_DISCONNECTED: /* Connection has been disconnected */
            ESP_LOGD(tag, "HTTP Event HTTP_EVENT_DISCONNECTED");
            chars_output = 0; /* Reset for allowing buffer to clear and for proper data appending to buffer */
            break;
        case HTTP_EVENT_ON_FINISH: /* HTTP session finished */
            ESP_LOGD(tag, "HTTP Event HTTP_EVENT_ON_FINISH");
            chars_output = 0; /* Reset for allowing buffer to clear and for proper data appending to buffer */
            break;
        default:
            /* Log event ID for debug */
            ESP_LOGD(tag, "HTTP Event %s", http_event_id_to_str(evt->event_id));
    }

    return ESP_OK;
}

/**
 * @brief Sends HTTPS request to configured Philips Hue bridge for controlling a light resource
 *
 * @param[in] p_light_data Pointer to structure with actions to perform on resource
 * @param[in] p_hue_config Pointer to structure with Philips Hue bridge information and application key for requests
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Successfully performed light resource action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – p_light_data or p_hue_config are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code not 200 OK or encoding error encountered
 * @retval - @c ESP_ERR_INVALID_SIZE – Request or URL buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
esp_err_t hue_light_https_request(hue_light_data_t* p_light_data, hue_config_t* p_hue_config) {
    if (HUE_NULL_CHECK(tag, p_light_data)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_hue_config)) return ESP_ERR_INVALID_ARG;
    hue_json_buffer_t json_buffer;
    esp_err_t err = ESP_OK;

    if ((err = hue_light_data_to_json(&json_buffer, p_light_data)) == ESP_OK) {
        err = hue_https_request(p_hue_config, &json_buffer);
    }

    return err;
}

/**
 * @brief Sends HTTPS request to configured Philips Hue bridge for controlling a grouped light resource
 *
 * @param[in] p_grouped_light_data Pointer to structure with actions to perform on resource
 * @param[in] p_hue_config Pointer to structure with Philips Hue bridge information and application key for requests
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Successfully performed grouped light resource action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – p_grouped_light_data or p_hue_config are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code not 200 OK or encoding error encountered
 * @retval - @c ESP_ERR_INVALID_SIZE – Request or URL buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
esp_err_t hue_grouped_light_https_request(hue_grouped_light_data_t* p_grouped_light_data, hue_config_t* p_hue_config) {
    if (HUE_NULL_CHECK(tag, p_grouped_light_data)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_hue_config)) return ESP_ERR_INVALID_ARG;
    hue_json_buffer_t json_buffer;
    esp_err_t err = ESP_OK;

    if ((err = hue_grouped_light_data_to_json(&json_buffer, p_grouped_light_data)) == ESP_OK) {
        err = hue_https_request(p_hue_config, &json_buffer);
    }

    return err;
}

/**
 * @brief Sends HTTPS request to configured Philips Hue bridge for controlling a smart scene resource
 *
 * @param[in] p_smart_scene_data Pointer to structure with actions to perform on resource
 * @param[in] p_hue_config Pointer to structure with Philips Hue bridge information and application key for requests
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Successfully performed smart scene resource action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – p_smart_scene_data or p_hue_config are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code not 200 OK or encoding error encountered
 * @retval - @c ESP_ERR_INVALID_SIZE – Request or URL buffer too small for HTTPS request or response
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
esp_err_t hue_smart_scene_https_request(hue_smart_scene_data_t* p_smart_scene_data, hue_config_t* p_hue_config) {
    if (HUE_NULL_CHECK(tag, p_smart_scene_data)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_hue_config)) return ESP_ERR_INVALID_ARG;
    hue_json_buffer_t json_buffer;
    esp_err_t err = ESP_OK;

    if ((err = hue_smart_scene_data_to_json(&json_buffer, p_smart_scene_data)) == ESP_OK) {
        err = hue_https_request(p_hue_config, &json_buffer);
    }

    return err;
}
