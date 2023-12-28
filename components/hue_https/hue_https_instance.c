/**
 * @file hue_https_instance.c
 * @author Tanner Baccus
 * @date 26 December 2023
 * @brief Implementation of all functions relating to the creation of Hue HTTPS instances
 */

#include "esp_log.h"

#include "hue_helpers.h"
#include "hue_https.h"
#include "hue_https_private.h"

static const char* tag = "hue_https_instance";

extern const char hue_signify_root_cert_pem_start[] asm("_binary_hue_signify_root_cert_pem_start");
extern const char hue_signify_root_cert_pem_end[] asm("_binary_hue_signify_root_cert_pem_end");

/*====================================================================================================================*/
/*========================================== Private Function Declarations ===========================================*/
/*====================================================================================================================*/

/**
 * @brief Function for perfoming current request to be looped in hue_https_send_request() for retrying
 *
 * @param[in,out] https_handle Handle for Hue HTTPS instance to send request under
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Request successfully performed
 * @retval - @c ESP_FAIL – Request aborted with WiFi disconnection, new request incoming, or Hue HTTPS instance exiting
 * @retval - @c ESP_ERR_INVALID_STATE – Client handle failed to be created
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Request successfully performed, but response status was not 200 OK
 * @retval - @c ESP_ERR_NOT_FINISHED – Request failed to perform and should be retried
 */
static esp_err_t hue_https_request_loop(hue_https_handle_t https_handle);

/**
 * @brief Performs request pointed to by the current_request_handle
 *
 * @param[in,out] https_handle Handle for Hue HTTPS instance to send request under
 */
static void hue_https_send_request(hue_https_handle_t https_handle);

/**
 * @brief FreeRTOS task function for performing requests without blocking from request calls
 *
 * @param[in,out] pvparameters Task required argument, should be passed as hue_https_handle_t
 */
static void hue_https_request_task(void* pvparameters);

/**
 * @brief Converts HTTP Event ID enum into string representation
 *
 * @param[in] id HTTP Event ID
 *
 * @return Human-readable string representation of HTTP Event ID
 */
static const char* http_event_id_to_str(esp_http_client_event_id_t id);

/**
 * @brief Event handler for HTTP session, as defined by esp_http_client
 *
 * @param[in] evt Event pointer
 *
 * @return ESP Error code, only returns ESP_OK, more research needed as to the purpose of this return
 */
static esp_err_t hue_https_event_handler(esp_http_client_event_t* evt);

/**
 * @brief Verifies that the Bridge IP is in IPV4 format
 *
 * @param[in] bridge_ip Bridge IP to check
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Bridge IP is formatted as expected
 * @retval - @c ESP_FAIL – Bridge IP is incorrectly formatted
 */
static esp_err_t check_bridge_ip(const char* bridge_ip);

/**
 * @brief Creates Hue HTTPS handle's url base with the specified IP address
 *
 * @param[out] hue_https_handle Hue HTTPS handle to print bridge_ip to
 * @param[in] bridge_ip IPV4 address as string to add to Hue HTTPS handle
 *
 * @return ESP Error Code
 * @retval - @c ESP_OK – URL base successfully printed to Hue HTTPS handle buffer
 * @retval - @c ESP_ERR_INVALID_ARG – hue_https_handle or bridge_ip is NULL or bridge_ip is not in expected format
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Encoding error occurred during printing
 * @retval - @c ESP_ERR_INVALID_SIZE – URL base printed was larger than expected
 */
static esp_err_t fill_bridge_url_base(hue_https_handle_t hue_https_handle, const char* bridge_ip);

/**
 * @brief Verifies that the Bridge ID is in the format specified by the Philips Hue API
 *
 * @param[in] bridge_id Bridge ID to check
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Bridge ID is formatted as expected
 * @retval - @c ESP_FAIL – Bridge ID is incorrectly formatted
 */
static esp_err_t check_bridge_id(const char* bridge_id);

/**
 * @brief Creates Hue HTTPS handle's Bridge ID with the specified Bridge ID
 *
 * @param[out] hue_https_handle Hue HTTPS handle to copy bridge_id to
 * @param[in] bridge_id Bridge ID to copy over to Hue HTTPS handle
 *
 * @return ESP Error Code
 * @retval - @c ESP_OK – Bridge ID successfully copied to Hue HTTPS handle buffer
 * @retval - @c ESP_ERR_INVALID_ARG – hue_https_handle or bridge_id is NULL or bridge_id is not in expected format
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Bridge ID copied failed Bridge ID check
 */
static esp_err_t fill_bridge_id(hue_https_handle_t hue_https_handle, const char* bridge_id);

/**
 * @brief Verifies that the Application Key is in the format specified by the Philips Hue API
 *
 * @param[in] app_key Application Key to check
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Application Key is formatted as expected
 * @retval - @c ESP_FAIL – Application Key is incorrectly formatted
 */
static esp_err_t check_app_key(const char* app_key);

/**
 * @brief Creates Hue HTTPS handle's Application Key with the specified Application Key
 *
 * @param[out] hue_https_handle Hue HTTPS handle to copy app_key to
 * @param[in] app_key Application Key to copy over to Hue HTTPS handle
 *
 * @return ESP Error Code
 * @retval - @c ESP_OK – Application Key successfully copied to Hue HTTPS handle buffer
 * @retval - @c ESP_ERR_INVALID_ARG – hue_https_handle or app_key is NULL or app_key is not in expected format
 * @retval - @c ESP_ERR_INVALID_RESPONSE – Application Key copied failed Application Key check
 */
static esp_err_t fill_app_key(hue_https_handle_t hue_https_handle, const char* app_key);

/**
 * @brief Frees all Hue HTTPS instance resources and sets handle to NULL
 *
 * @param[in,out] p_hue_https_handle Pointer to Hue HTTPS instance handle (value will be set to NULL after)
 *
 * @note p_hue_https_handle is a pointer to a pointer to a Hue HTTPS instance, this is used to force the handle to be
 * set to NULL so deallocated memory cannot be accessed with the handle
 */
static void free_hue_https_instance(hue_https_handle_t* p_hue_https_handle);

/**
 * @brief Allocates all memory for Hue HTTPS instance and defines all internal values from config p_hue_https_config
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
 * @retval - @c ESP_ERR_NO_MEM – Failed to allocate memory or to create Event Group or Mutex for Hue HTTPS instance
 */
static esp_err_t alloc_hue_https_instance(hue_https_handle_t* p_hue_https_handle,
                                          hue_https_config_t* p_hue_https_config);

/*====================================================================================================================*/
/*=========================================== Public Function Definitions ============================================*/
/*====================================================================================================================*/

esp_err_t hue_https_create_instance(hue_https_handle_t* p_hue_https_handle, hue_https_config_t* p_hue_https_config) {
    if (HUE_NULL_CHECK(tag, p_hue_https_handle)) return ESP_ERR_INVALID_ARG;
    if (*p_hue_https_handle) {
        ESP_LOGE(tag, "Hue HTTPS handle already created, destroy previous handle before re-creating");
        return ESP_ERR_INVALID_ARG;
    }
    if (HUE_NULL_CHECK(tag, p_hue_https_config)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_hue_https_config->bridge_ip)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_hue_https_config->bridge_id)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_hue_https_config->application_key)) return ESP_ERR_INVALID_ARG;

    /* Verify that all config strings are in specified format before continuing */
    if (check_bridge_ip(p_hue_https_config->bridge_ip) != ESP_OK) return ESP_ERR_INVALID_ARG;
    if (check_bridge_id(p_hue_https_config->bridge_id) != ESP_OK) return ESP_ERR_INVALID_ARG;
    if (check_app_key(p_hue_https_config->application_key) != ESP_OK) return ESP_ERR_INVALID_ARG;

    /* Allocate all resources needed for instance and return if an error is encountered */
    esp_err_t err = alloc_hue_https_instance(p_hue_https_handle, p_hue_https_config);
    if (err != ESP_OK) return err;

    /* Create task for main loop and return if an error is encountered */
    if (xTaskCreate(hue_https_request_task, p_hue_https_config->task_id, 8192, *p_hue_https_handle,
                    configMAX_PRIORITIES - 5, &((*p_hue_https_handle)->task_handle)) != pdPASS) {
        ESP_LOGE(tag, "Failed to create Hue HTTPS instance task");
        free_hue_https_instance(p_hue_https_handle);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

/* TODO: Instance destroy */
esp_err_t hue_https_destroy_instance(hue_https_handle_t* p_hue_https_handle) {return ESP_ERR_NOT_FINISHED;}

/*====================================================================================================================*/
/*=========================================== Private Function Definitions ===========================================*/
/*====================================================================================================================*/

static esp_err_t hue_https_request_loop(hue_https_handle_t https_handle) {
    EventBits_t bits = xEventGroupGetBits(https_handle->handle_evt);
    esp_err_t err = ESP_FAIL;

    /* Return ESP_FAIL if Connected Bit is not set or Abort/Exit Bits are set*/
    if ((bits ^ HUE_HTTPS_EVT_WIFI_CONNECTED_BIT) & HUE_HTTPS_EVT_ABORT_BIT & HUE_HTTPS_EVT_EXIT_BIT) return ESP_FAIL;
    esp_http_client_handle_t client = esp_http_client_init(&(https_handle->client_config));

    if (!client) {
        ESP_LOGE(tag, "Client handle failed to be created");
        return ESP_ERR_INVALID_STATE;
    }

    /* Set headers used by Hue API */
    esp_http_client_set_header(client, "hue-application-key", https_handle->app_key);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    /* Add generated actions to request */
    char* body = https_handle->current_request_handle->request_body;
    esp_http_client_set_post_field(client, body, strlen(body));

    if ((err = esp_http_client_perform(client)) == ESP_OK) {
        /* If response status code is not 200 OK, log actual status and set for return ESP_ERR_INVALID_RESPONSE */
        if (esp_http_client_get_status_code(client) != HttpStatus_Ok) {
            ESP_LOGE(tag, "HTTP response status not 200 OK, recieved %d", esp_http_client_get_status_code(client));
            err = ESP_ERR_INVALID_RESPONSE;
        }
    } else {
        err = ESP_ERR_NOT_FINISHED;
    }

    /* Clean up connection and return ESP_OK, ESP_ERR_INVALID_RESPONSE, or ESP_ERR_NOT_FINISHED */
    esp_http_client_cleanup(client);
    return err;
}

static void hue_https_send_request(hue_https_handle_t https_handle) {
    if (!https_handle) return;
    if (!(https_handle->current_request_handle)) return;
    if (!(https_handle->current_request_handle->resource_path)) return;
    if (!(https_handle->current_request_handle->request_body)) return;

    uint8_t url_res_pos = https_handle->url_res_path_pos;
    if ((url_res_pos > HUE_URL_BASE_MAX_LENGTH) || (url_res_pos < HUE_URL_BASE_MIN_LENGTH)) return;

    /* Add resource path to URL */
    char* res_path = https_handle->current_request_handle->resource_path;
    strncpy(&(https_handle->buff_url[url_res_pos]), res_path, HUE_URL_BUFFER_SIZE - url_res_pos);

    esp_err_t err;
    uint8_t attempt_num = 0;

    /* Retry request perform until the max attempts have been reached or until ESP_ERR_NOT_FINISHED is not returned */
    while (attempt_num <= (https_handle->retry_attempts)) {
        err = hue_https_request_loop(https_handle);
        if (err != ESP_ERR_NOT_FINISHED) break;
        attempt_num++;
        ESP_LOGI(tag, "Request attempt #%d failed, %s", attempt_num,
                 (attempt_num <= (https_handle->retry_attempts) ? "retrying" : "max attempts reached"));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* Protect request handles with mutex */
    if (xSemaphoreTake(https_handle->request_handle_mutex, portMAX_DELAY)) {
        /* Move next request to current */
        https_handle->current_request_handle = https_handle->next_request_handle;
        https_handle->next_request_handle = NULL;

        /* If another request was pending, set the trigger event bit to start the next request */
        if (https_handle->current_request_handle) {
            xEventGroupSetBits(https_handle->handle_evt, HUE_HTTPS_EVT_TRIGGER_BIT);
        }

        /* Clear abort bit if it was set */
        xEventGroupClearBits(https_handle->handle_evt, HUE_HTTPS_EVT_ABORT_BIT);
    }
    xSemaphoreGive(https_handle->request_handle_mutex);
}

static void hue_https_request_task(void* pvparameters) {
    if (HUE_NULL_CHECK(tag, pvparameters)) vTaskDelete(NULL);

    hue_https_handle_t https_handle = (hue_https_handle_t)pvparameters;
    EventBits_t bits;

    while (true) {
        bits = xEventGroupWaitBits(https_handle->handle_evt, HUE_HTTPS_EVT_WAIT_BITS, pdFALSE, pdFALSE, portMAX_DELAY);
        if (bits & HUE_HTTPS_EVT_EXIT_BIT) break;
        if (!(bits & (HUE_HTTPS_EVT_WIFI_CONNECTED_BIT | HUE_HTTPS_EVT_TRIGGER_BIT))) continue;
        hue_https_send_request(https_handle);
    }

    vTaskDelete(NULL);
}

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

static esp_err_t hue_https_event_handler(esp_http_client_event_t* evt) {
    static size_t chars_output = 0; /* Number of characters output to buffer */

    switch (evt->event_id) {
        case HTTP_EVENT_ON_HEADER: /* Header recieved from server */
            ESP_LOGD(tag, "HTTP Event HTTP_EVENT_ON_HEADER, %s: %s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA: /* Data recieved from server */
            ESP_LOGD(tag, "HTTP Event HTTP_EVENT_ON_DATA\n\tData length = %d\n\t%.*s", evt->data_len, evt->data_len - 2,
                     (char*)evt->data);
            if (!evt->user_data) break;

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

static esp_err_t check_bridge_ip(const char* bridge_ip) {
    if (strlen(bridge_ip) != HUE_BRIDGE_IP_LENGTH) {
        ESP_LOGE(tag, "Bridge IP provided is not the correct length for an IPV4 address");
        return ESP_FAIL;
    }

    /* Storage for the number of correctly formatted characters scanned */
    int chars_received = 0;

    /* Scans the Bridge IP against the specified format, storing the number of characters scanned */
    sscanf(bridge_ip, HUE_BRIDGE_IP_FORMAT "%n", &chars_received);

    /* All characters should match the specified format */
    if (chars_received != HUE_BRIDGE_IP_LENGTH) {
        ESP_LOGE(tag, "Bridge IP provided is not in the correct format for an IPV4 address");
        return ESP_FAIL;
    }

    /* Bridge IP passed the check */
    return ESP_OK;
}

static esp_err_t fill_bridge_url_base(hue_https_handle_t hue_https_handle, const char* bridge_ip) {
    if (HUE_NULL_CHECK(tag, hue_https_handle)) return ESP_ERR_INVALID_ARG;

    /* Redundant checking to ensure string has not been modified before copying */
    if (check_bridge_ip(bridge_ip) != ESP_OK) return ESP_ERR_INVALID_ARG;

    /* Ensure that buffer is clear */
    memset(hue_https_handle->buff_url, 0, HUE_URL_BUFFER_SIZE);

    /* Print URL base using set IP address */
    esp_err_t err = snprintf(hue_https_handle->buff_url, HUE_URL_BASE_SIZE, "https://%s" HUE_RESOURCE_PATH, bridge_ip);

    /* Verify that URL base is within expected length */
    if (err < 0) {
        ESP_LOGE(tag, "URL string printing encoding failure");
        return ESP_ERR_INVALID_RESPONSE;
    }
    if ((err < HUE_URL_BASE_MIN_LENGTH) || (err > HUE_URL_BASE_MAX_LENGTH)) {
        ESP_LOGE(tag, "URL base is out of the expected length");
        return ESP_ERR_INVALID_SIZE;
    }

    /* Set the index for the resource path to append to the URL base */
    hue_https_handle->url_res_path_pos = strlen(hue_https_handle->buff_url);

    return ESP_OK;
}

static esp_err_t check_bridge_id(const char* bridge_id) {
    if (strlen(bridge_id) != HUE_BRIDGE_ID_LENGTH) {
        ESP_LOGE(tag, "Bridge ID provided is not the correct length for a Bridge ID");
        return ESP_FAIL;
    }

    /* Storage for the number of correctly formatted characters scanned */
    int chars_received = 0;

    /* Scans the Bridge ID against the specified format, storing the number of characters scanned */
    sscanf(bridge_id, HUE_BRIDGE_ID_FORMAT "%n", &chars_received);

    /* All characters should match the specified format */
    if (chars_received != HUE_BRIDGE_ID_LENGTH) {
        ESP_LOGE(tag, "Bridge ID provided is not in the correct format for a Bridge ID");
        return ESP_FAIL;
    }

    /* Bridge ID passed the check */
    return ESP_OK;
}

static esp_err_t fill_bridge_id(hue_https_handle_t hue_https_handle, const char* bridge_id) {
    if (HUE_NULL_CHECK(tag, hue_https_handle)) return ESP_ERR_INVALID_ARG;

    /* Redundant checking to ensure string has not been modified before copying */
    if (check_bridge_id(bridge_id) != ESP_OK) return ESP_ERR_INVALID_ARG;

    /* Copy set Bridge ID to instance buffer */
    strncpy(hue_https_handle->bridge_id, bridge_id, HUE_BRIDGE_ID_LENGTH);

    /* Verify that copied Bridge ID is of the expected format */
    if (check_bridge_id(hue_https_handle->bridge_id) != ESP_OK) return ESP_ERR_INVALID_RESPONSE;

    return ESP_OK;
}

static esp_err_t check_app_key(const char* app_key) {
    if (strlen(app_key) != HUE_APPLICATION_KEY_LENGTH) {
        ESP_LOGE(tag, "Application Key provided is not the correct length for an Application Key");
        return ESP_FAIL;
    }

    /* Storage for the number of correctly formatted characters scanned */
    int chars_received = 0;

    /* Scans the Application Key against the specified format, storing the number of characters scanned */
    sscanf(app_key, HUE_APPLICATION_KEY_FORMAT "%n", &chars_received);

    /* All characters should match the specified format */
    if (chars_received != HUE_APPLICATION_KEY_LENGTH) {
        ESP_LOGE(tag, "Application Key provided is not in the correct format for an Application Key");
        return ESP_FAIL;
    }

    /* Application Key passed the check */
    return ESP_OK;
}

static esp_err_t fill_app_key(hue_https_handle_t hue_https_handle, const char* app_key) {
    if (HUE_NULL_CHECK(tag, hue_https_handle)) return ESP_ERR_INVALID_ARG;

    /* Redundant checking to ensure string has not been modified before copying */
    if (check_app_key(app_key) != ESP_OK) return ESP_ERR_INVALID_ARG;

    /* Copy set Application Key to instance buffer */
    strncpy(hue_https_handle->app_key, app_key, HUE_APPLICATION_KEY_LENGTH);

    /* Verify that copied Application Key is of the expected format */
    if (check_app_key(hue_https_handle->app_key) != ESP_OK) return ESP_ERR_INVALID_RESPONSE;

    return ESP_OK;
}

static void free_hue_https_instance(hue_https_handle_t* p_hue_https_handle) {
    /* If p_hue_https_handle or Hue HTTPS handle are already NULL, nothing needs to be done */
    if (!p_hue_https_handle) return;
    if (!(*p_hue_https_handle)) return;

    /* Free any resources that are allocated */
    if ((*p_hue_https_handle)->task_handle) vTaskDelete((*p_hue_https_handle)->task_handle);
    if ((*p_hue_https_handle)->handle_evt) vEventGroupDelete((*p_hue_https_handle)->handle_evt);
    if ((*p_hue_https_handle)->request_handle_mutex) vSemaphoreDelete((*p_hue_https_handle)->request_handle_mutex);

    /* Free the request instance */
    free(*p_hue_https_handle);

    /* Sets the value of the request handle to NULL to ensure handle cannot be used to access deallocated memory */
    *p_hue_https_handle = NULL;
}

static esp_err_t alloc_hue_https_instance(hue_https_handle_t* p_hue_https_handle,
                                          hue_https_config_t* p_hue_https_config) {
    if (HUE_NULL_CHECK(tag, p_hue_https_handle)) return ESP_ERR_INVALID_ARG;
    if (HUE_NULL_CHECK(tag, p_hue_https_config)) return ESP_ERR_INVALID_ARG;

    /* Allocate memory for the Hue HTTPS instance and set handle value to the instance pointer */
    (*p_hue_https_handle) = malloc(sizeof(hue_https_instance_t));
    if (!(*p_hue_https_handle)) {
        ESP_LOGE(tag, "Failed to allocate memory for Hue HTTPS instance");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err;

    /* Fill buff_url with URL base and set url_resource_path pointer, returning if error encountered */
    if ((err = fill_bridge_url_base(*p_hue_https_handle, p_hue_https_config->bridge_ip)) != ESP_OK) {
        free_hue_https_instance(p_hue_https_handle);
        return err;
    }

    /* Fill bridge_id, returning if error encountered */
    if ((err = fill_bridge_id(*p_hue_https_handle, p_hue_https_config->bridge_id)) != ESP_OK) {
        free_hue_https_instance(p_hue_https_handle);
        return err;
    }

    /* Fill app_key, returning if error encountered */
    if ((err = fill_app_key(*p_hue_https_handle, p_hue_https_config->application_key)) != ESP_OK) {
        free_hue_https_instance(p_hue_https_handle);
        return err;
    }

    /* Create Event Group and Request Handle Mutex */
    if (((*p_hue_https_handle)->handle_evt = xEventGroupCreate()) == NULL) {
        ESP_LOGE(tag, "Failed to create Event Group");
        free_hue_https_instance(p_hue_https_handle);
        return ESP_ERR_NO_MEM;
    }
    if (((*p_hue_https_handle)->request_handle_mutex = xSemaphoreCreateMutex()) == NULL) {
        ESP_LOGE(tag, "Failed to create mutex");
        free_hue_https_instance(p_hue_https_handle);
        return ESP_ERR_NO_MEM;
    }

    /* Set all undefined pointers to NULL */
    (*p_hue_https_handle)->current_request_handle = NULL;
    (*p_hue_https_handle)->next_request_handle = NULL;
    (*p_hue_https_handle)->task_handle = NULL;

    /* Set up HTTP Client Config */
    (*p_hue_https_handle)->client_config.url = (*p_hue_https_handle)->buff_url;
    (*p_hue_https_handle)->client_config.cert_pem = hue_signify_root_cert_pem_start;     /* CA Cert for TLS */
    (*p_hue_https_handle)->client_config.common_name = (*p_hue_https_handle)->bridge_id; /* CN for TLS verification */
    (*p_hue_https_handle)->client_config.event_handler = hue_https_event_handler;
    (*p_hue_https_handle)->client_config.timeout_ms = 5000;        /* Max time to attempt request before failing */
    (*p_hue_https_handle)->client_config.method = HTTP_METHOD_PUT; /* Currently only PUT requests implemented */

    (*p_hue_https_handle)->retry_attempts = p_hue_https_config->retry_attempts;

    return ESP_OK;
}