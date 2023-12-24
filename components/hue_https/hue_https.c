#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_log.h"

#include "hue_https.h"
#include "hue_helpers.h"

static const char* tag = "hue_https";

extern const char hue_signify_root_cert_pem_start[] asm("_binary_hue_signify_root_cert_pem_start");
extern const char hue_signify_root_cert_pem_end[] asm("_binary_hue_signify_root_cert_pem_end");

/** @brief Storage for all required data for hue_https instance */
typedef struct hue_https_instance {
    TaskHandle_t task_handle;      /**< Task handle for performing requests with instance */
    EventGroupHandle_t handle_evt; /**< Event group for communication from request instances to https instance */

    char buff_url[HUE_URL_BUFFER_SIZE];           /**< Buffer for request URL */
    char app_key[HUE_APPLICATION_KEY_LENGTH + 1]; /**< Application key needed for requests */
    esp_http_client_config_t client_config;       /**< Config for http clients under this instance */

    SemaphoreHandle_t request_handle_mutex;            /**< Protects request handles from parallel tasks */
    hue_https_request_handle_t current_request_handle; /**< Handle for request being performed */
    hue_https_request_handle_t next_request_handle;    /**< Handle for request to replace current */
} hue_https_instance_t;

/** @brief Storage for HTTP request body and URL resource path */
typedef struct hue_https_request_instance {
    char* request_body;  /**< Body of HTTP request storing Philips Hue actions */
    char* resource_path; /**< URL path to resource with resource type and ID */
} hue_https_request_instance_t;

static void create_event_group_handle();

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
static esp_err_t hue_https_request();

/**
 * @brief Event handler for HTTP session, as defined by esp_http_client
 *
 * @param[in] evt Event pointer
 *
 * @return ESP Error code, only returns ESP_OK, more research needed as to the purpose of this return
 */
static esp_err_t hue_https_event_handler(esp_http_client_event_t* evt);

static void create_event_group_handle() {
    if (!event_group_handle) {
        event_group_handle = xEventGroupCreateStatic(&event_group_buff);
    }
}

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
static esp_err_t hue_https_request() {
    if (HUE_NULL_CHECK(tag, hue_config)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, hue_config->bridge_id)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, hue_config->application_key)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer.buff)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer.resource_id)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, json_buffer.resource_type)) return ESP_ERR_INVALID_ARG;

    /* Create URL for bridge resource using set IP and resource information */
    esp_err_t err = snprintf(url_str, HUE_URL_BUFFER_SIZE, "https://%d.%d.%d.%d" HUE_RESOURCE_PATH "%s/%s",
                             hue_config->bridge_ip[0], hue_config->bridge_ip[1], hue_config->bridge_ip[2],
                             hue_config->bridge_ip[3], json_buffer.resource_type, json_buffer.resource_id);

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
    esp_http_client_config_t config = {.url = url_str,
                                       .cert_pem = hue_signify_root_cert_pem_start, /* CA cert used for TLS */
                                       .common_name = hue_config->bridge_id,        /* CN used to verify CA cert */
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
        esp_http_client_set_post_field(client, json_buffer.buff, strlen(json_buffer.buff));

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

            /* If no data has been output, clear output buffer */
            if (chars_output == 0) memset(request_buffer, 0, HUE_REQUEST_BUFFER_SIZE);

            /* Append output data to buffer, ensuring data cannot overflow the buffer */
            if ((HUE_REQUEST_BUFFER_SIZE - 1 - chars_output) > 0) {
                strncat(request_buffer, evt->data, HUE_REQUEST_BUFFER_SIZE - 1 - chars_output);
                chars_output = strlen(request_buffer); /* Update the number of characters output */
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
esp_err_t hue_light_https_request(hue_light_data_t* p_light_data) {
    if (HUE_NULL_CHECK(tag, p_light_data)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_hue_config)) return ESP_ERR_INVALID_ARG;
    create_event_group_handle();
    esp_err_t err = ESP_OK;

    if ((err = hue_light_data_to_json(&json_buffer, p_light_data)) == ESP_OK) {
        err = hue_https_request(p_hue_config);
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
esp_err_t hue_grouped_light_https_request(hue_grouped_light_data_t* p_grouped_light_data) {
    if (HUE_NULL_CHECK(tag, p_grouped_light_data)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_hue_config)) return ESP_ERR_INVALID_ARG;
    create_event_group_handle();
    esp_err_t err = ESP_OK;

    if ((err = hue_grouped_light_data_to_json(&json_buffer, p_grouped_light_data)) == ESP_OK) {
        err = hue_https_request(p_hue_config);
    }

    return err;
}

/**
 * @brief Sends HTTPS request to configured Philips Hue bridge for controlling a smart scene resource
 *
 * @param[in] p_smart_scene_data Pointer to structure with actions to perform on resource
 * @param[in] p_hue_config Pointer to structure with Philips Hue bridge information and application key for requests
 * @param[in] timeout Max time (0 for max delay) in ms to wait for hue_https_report_wifi_connected() to be called before
 * returning @c ESP_ERR_WIFI_NOT_CONNECT
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Successfully performed smart scene resource action(s)
 * @retval - @c ESP_ERR_INVALID_ARG – p_smart_scene_data or p_hue_config are NULL
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Response code not 200 OK or encoding error encountered
 * @retval - @c ESP_ERR_INVALID_SIZE – Request or URL buffer too small for HTTPS request or response
 * @retval - @c ESP_ERR_INVALID_STATE – A request has already been started, which is now being aborted
 * @retval - @c ESP_ERR_WIFI_NOT_CONNECT – WiFi has not been indicated to be connected with
 * hue_https_report_wifi_connected()
 * @retval - @c ESP_FAIL – Request failed after retrying [retry_attempts] times
 */
esp_err_t hue_smart_scene_https_request(hue_smart_scene_data_t* p_smart_scene_data, uint32_t timeout) {
    if (HUE_NULL_CHECK(tag, p_smart_scene_data)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_hue_config)) return ESP_ERR_INVALID_ARG;
    create_event_group_handle();
    esp_err_t err = ESP_OK;

    if (xEventGroupGetBits(event_group_handle) & HUE_HTTPS_EVT_LOCK_BIT) {
        xEventGroupSetBits(event_group_handle, HUE_HTTPS_EVT_ABORT_BIT);
        return ESP_ERR_INVALID_STATE;
    }

    xEventGroupSetBits(event_group_handle, HUE_HTTPS_EVT_LOCK_BIT);
    if ((err = hue_smart_scene_data_to_json(&json_buffer, p_smart_scene_data)) == ESP_OK) {
        err = xEventGroupWaitBits(event_group_handle, HUE_HTTPS_EVT_WIFI_CONNECTED_BIT, pdFALSE, pdFALSE,
                                  (timeout == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout));
        if (err & HUE_HTTPS_EVT_WIFI_CONNECTED_BIT) {
            err = hue_https_request(p_hue_config);
        } else {
            err = ESP_ERR_WIFI_NOT_CONNECT;
        }
    }
    xEventGroupClearBits(event_group_handle, HUE_HTTPS_EVT_LOCK_BIT);

    return err;
}

/* TODO: Finish request restructuring   *
 * Currently restructured code is below */

/**
 * @brief Verifies that resource ID is in the format specified by the Philips Hue API
 *
 * @param[in] resource_id Resource ID to check
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Resource ID formatted as expected
 * @retval - @c ESP_FAIL – Resource ID incorrectly formatted
 */
static esp_err_t hue_check_resource_id(char* resource_id) {
    if (strlen(resource_id) != HUE_RESOURCE_ID_LENGTH) {
        ESP_LOGE(tag, "Resource ID provided is not the correct length for a resource ID");
        return ESP_FAIL;
    }

    /* Storage for the number of correctly formatted characters scanned */
    int chars_received = 0;

    /* Scans the resource ID against the specified format, storing the number of characters scanned */
    sscanf(resource_id, HUE_RESOURCE_ID_FORMAT "%n", &chars_received);

    /* All characters should match the specified format */
    if (chars_received != HUE_RESOURCE_ID_LENGTH) {
        ESP_LOGE(tag, "Resource ID provided is not in the correct format for a resource ID");
        return ESP_FAIL;
    }

    /* Resource ID passed the check */
    return ESP_OK;
}

/**
 * @brief Frees all request instance resources and sets handle to NULL
 *
 * @param[in,out] p_request_handle Pointer to request instance handle (value will be set to NULL after)
 *
 * @note p_request_handle is a pointer to a pointer to a request instance, this is used to force the handle to be set to
 * NULL so deallocated memory cannot be accessed with the handle
 */
static void hue_https_free_request(hue_https_request_handle_t* p_request_handle) {
    /* If p_request_handle or request handle are already NULL, nothing needs to be done */
    if (!p_request_handle) return;
    if (!(*p_request_handle)) return;

    /* Free any resources that are allocated */
    if ((*p_request_handle)->request_body) free((*p_request_handle)->request_body);
    if ((*p_request_handle)->resource_path) free((*p_request_handle)->resource_path);

    /* Free the request instance */
    free(*p_request_handle);

    /* Sets the value of the request handle to NULL to ensure handle cannot be used to access deallocated memory */
    *p_request_handle = NULL;
}

/**
 * @brief Allocates memory for the body of the HTTP request based on the size of the JSON in the buffer and copies over
 * the JSON from the buffer
 *
 * @param[in,out] request_handle Handle for the request instance to create the body for
 * @param[in] p_json_buffer JSON buffer filled by hue_json_builder from hue data structure
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Request body successfully allocated and filled
 * @retval - @c ESP_ERR_INVALID_ARG – request_handle, p_json_buffer, or p_json_buffer's internal buffer are NULL
 * @retval - @c ESP_ERR_INVALID_SIZE – Data in p_json_buffer is empty/out of buffer range or JSON did not copy correctly
 * @retval - @c ESP_ERR_NO_MEM – Failed to allocate memory for request body
 */
static esp_err_t hue_https_allocate_request_body(hue_https_request_handle_t request_handle,
                                                 hue_json_buffer_t* p_json_buffer) {
    if (HUE_NULL_CHECK(tag, request_handle)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_json_buffer)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_json_buffer->buff)) return ESP_ERR_INVALID_ARG;

    /* Get the size of the JSON generated for the request */
    size_t str_len = strlen(p_json_buffer->buff);

    if (str_len == 0) { /* Buffer exists but is empty */
        ESP_LOGE(tag, "JSON buffer not filled");
        return ESP_ERR_INVALID_SIZE;
    } else if (str_len >= HUE_JSON_BUFFER_SIZE) { /* Prevent copying out of bounds without null-terminating character */
        ESP_LOGE(tag, "JSON buffer has no null-terminating character");
        return ESP_ERR_INVALID_SIZE;
    }

    /* If request body is already allocated, free before overwriting pointer */
    if (request_handle->request_body) free(request_handle->request_body);

    /* Allocate request body to exactly fit the JSON generated for the request */
    request_handle->request_body = calloc(str_len + 1, sizeof(char));
    if (!request_handle->request_body) {
        ESP_LOGE(tag, "Failed to allocate memory for request body");
        return ESP_ERR_NO_MEM;
    }

    /* Copy the generated JSON over to the request body buffer */
    strncpy(request_handle->request_body, p_json_buffer->buff, str_len);

    /* Redundant checking to ensure that JSON has been fully copied */
    if (unlikely(strlen(request_handle->request_body) != str_len)) {
        ESP_LOGE(tag, "Failed to copy JSON to request handle");
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

/**
 * @brief Allocates memory for the resource path of the HTTP request URL based on the size of the resource type and ID
 * strings stored in p_json_buffer
 *
 * @param[in,out] request_handle Handle for the request instance to create the resource path for
 * @param[in] p_json_buffer JSON buffer filled by hue_json_builder from hue data structure
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Resource path successfully allocated and filled
 * @retval - @c ESP_ERR_INVALID_ARG – request_handle, p_json_buffer, or resource type/ID from p_json_buffer are NULL
 * @retval - @c ESP_ERR_INVALID_SIZE – Resource type/ID strings are of unexpected length or resource path print failed
 * @retval - @c ESP_ERR_NO_MEM – Failed to allocate memory for resource
 */
static esp_err_t hue_https_allocate_resource_path(hue_https_request_handle_t request_handle,
                                                  hue_json_buffer_t* p_json_buffer) {
    if (HUE_NULL_CHECK(tag, request_handle)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_json_buffer)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_json_buffer->resource_type)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_json_buffer->resource_id)) return ESP_ERR_INVALID_ARG;

    /* Get the size of the resource type + '/' + resource ID */
    size_t str_len = strlen(p_json_buffer->resource_type) + 1 + strlen(p_json_buffer->resource_id);

    /* Resource ID should be verified by this point & resource type should be one of 4 static strings, so unlikely to be
     * improperly sized, but check for improper lengths anyway */
    if (unlikely(str_len < (HUE_RESOURCE_TYPE_MIN + HUE_RESOURCE_ID_LENGTH))) {
        ESP_LOGE(tag, "Resource type and/or ID are too short");
        return ESP_ERR_INVALID_SIZE;
    }
    if (unlikely(str_len > (HUE_RESOURCE_TYPE_SIZE + HUE_RESOURCE_ID_LENGTH))) {
        ESP_LOGE(tag, "Resource type and/or ID are too long");
        return ESP_ERR_INVALID_SIZE;
    }

    /* If resource path is already allocated, free before overwriting pointer */
    if (request_handle->resource_path) free(request_handle->resource_path);

    /* Allocate resource path to exactly fit "[resource type]/[resource id]" */
    request_handle->resource_path = calloc(str_len + 1, sizeof(char));
    if (!request_handle->resource_path) {
        ESP_LOGE(tag, "Failed to allocate memory for resource path");
        return ESP_ERR_NO_MEM;
    }

    /* Put "[resource type]/[resource id]" into request resource path */
    int err = snprintf(request_handle->resource_path, str_len + 1, "%s/%s", p_json_buffer->resource_type,
                       p_json_buffer->resource_id);

    /* Verify that resource path has been properly put into buffer */
    if (unlikely((err != str_len) || (strlen(request_handle->resource_path) != str_len))) {
        ESP_LOGE(tag, "Failed to put resource path into request buffer");
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

/**
 * @brief Allocates all memory for request instance and fills the request body and resource path buffers with data from
 * p_json_buffer
 *
 * @param[out] p_request_handle Request handle to store instance into to be used with hue https instance
 * @param[in] p_json_buffer JSON generated by hue_json_builder using hue data structure
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Request instance successfully allocated and filled
 * @retval - @c ESP_ERR_INVALID_ARG – p_request_handle, p_json_buffer, or p_json_buffer's internal pointers are NULL
 * @retval - @c ESP_ERR_INVALID_SIZE – Data in p_json_buffer is out of expected bounds or request instance buffers
 * failed to be copied to
 * @retval - @c ESP_ERR_NO_MEM – Failed to allocate memory for request instance
 */
static esp_err_t hue_https_allocate_request(hue_https_request_handle_t* p_request_handle,
                                            hue_json_buffer_t* p_json_buffer) {
    if (HUE_NULL_CHECK(tag, p_request_handle)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_json_buffer)) return ESP_ERR_INVALID_ARG;

    /* Allocate memory for the request instance and set handle value to the instance pointer */
    (*p_request_handle) = malloc(sizeof(hue_https_request_instance_t));
    if (!(*p_request_handle)) {
        ESP_LOGE(tag, "Failed to allocate memory for request instance");
        return ESP_ERR_NO_MEM;
    }

    /* Set internal unallocated buffer pointers to NULL */
    (*p_request_handle)->request_body = NULL;
    (*p_request_handle)->resource_path = NULL;

    /* Allocate and fill request body buffer from p_json_buffer */
    esp_err_t err = hue_https_allocate_request_body(*p_request_handle, p_json_buffer);
    if (err != ESP_OK) {
        /* Free all potentially allocated resources before returning allocation error code */
        hue_https_free_request(p_request_handle);
        return err;
    }

    /* Allocate and fill resource path buffer from p_json_buffer */
    err = hue_https_allocate_resource_path(*p_request_handle, p_json_buffer);
    if (err != ESP_OK) {
        /* Free all potentially allocated resources before returning allocation error code */
        hue_https_free_request(p_request_handle);
        return err;
    }

    return ESP_OK;
}

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
esp_err_t hue_https_create_light_request(hue_https_request_handle_t* p_request_handle, hue_light_data_t* p_light_data) {
    if (HUE_NULL_CHECK(tag, p_request_handle)) return ESP_ERR_INVALID_ARG;
    if (*p_request_handle) {
        ESP_LOGE(tag, "Request handle already created, destroy previous handle before re-creating");
        return ESP_ERR_INVALID_ARG;
    }
    if (HUE_NULL_CHECK(tag, p_light_data)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_light_data->resource_id)) return ESP_ERR_INVALID_ARG;

    /* Verify that resource ID given is in the correct format */
    if (hue_check_resource_id(p_light_data->resource_id) != ESP_OK) return ESP_ERR_INVALID_ARG;

    /* Generate JSON from p_light_data */
    hue_json_buffer_t json_buffer;
    esp_err_t err = hue_light_data_to_json(&json_buffer, p_light_data);
    if (err != ESP_OK) return err;

    /* Allocate and fill request instance */
    return hue_https_allocate_request(p_request_handle, &json_buffer);
}