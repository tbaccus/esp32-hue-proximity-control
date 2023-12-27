/**
 * @file hue_https_instance.c
 * @author Tanner Baccus
 * @date 26 December 2023
 * @brief Implementation of all functions relating to the creation of Hue HTTPS instances
 */
#include "esp_log.h"

#include "hue_https.h"
#include "hue_https_private.h"

static const char* tag = "hue_https_instance";

extern const char hue_signify_root_cert_pem_start[] asm("_binary_hue_signify_root_cert_pem_start");
extern const char hue_signify_root_cert_pem_end[] asm("_binary_hue_signify_root_cert_pem_end");

/*====================================================================================================================*/
/*========================================== Private Function Declarations ===========================================*/
/*====================================================================================================================*/

static void hue_https_request_task(void* pvparameters);

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
static esp_err_t check_bridge_ip(char* bridge_ip);

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
static esp_err_t fill_bridge_url_base(hue_https_handle_t hue_https_handle, char* bridge_ip);

/**
 * @brief Verifies that the Bridge ID is in the format specified by the Philips Hue API
 *
 * @param[in] bridge_id Bridge ID to check
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Bridge ID is formatted as expected
 * @retval - @c ESP_FAIL – Bridge ID is incorrectly formatted
 */
static esp_err_t check_bridge_id(char* bridge_id);

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
static esp_err_t fill_bridge_id(hue_https_handle_t hue_https_handle, char* bridge_id);

/**
 * @brief Verifies that the Application Key is in the format specified by the Philips Hue API
 *
 * @param[in] app_key Application Key to check
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – Application Key is formatted as expected
 * @retval - @c ESP_FAIL – Application Key is incorrectly formatted
 */
static esp_err_t check_app_key(char* app_key);

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
static esp_err_t fill_app_key(hue_https_handle_t hue_https_handle, char* app_key);

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
    if(err != ESP_OK) return err;

    /* Create task for main loop and return if an error is encountered */
    if(xTaskCreate(hue_https_request_task, p_hue_https_config->task_id, 8192, *p_hue_https_handle, configMAX_PRIORITIES - 5, &((*p_hue_https_handle)->task_handle)) != pdPASS) {
        ESP_LOGE(tag, "Failed to create Hue HTTPS instance task");
        free_hue_https_instance(p_hue_https_handle);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

/* TODO: Instance destroy */
esp_err_t hue_https_destroy_instance(hue_https_handle_t* p_hue_https_handle) {}

/*====================================================================================================================*/
/*=========================================== Private Function Definitions ===========================================*/
/*====================================================================================================================*/


/* TODO: Request task */
static void hue_https_request_task(void* pvparameters) {
    if (HUE_NULL_CHECK(pvparameters)) vTaskDelete(NULL);

    hue_https_handle_t https_handle = (hue_https_handle_t)pvparameters;
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

static esp_err_t check_bridge_ip(char* bridge_ip) {
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

static esp_err_t fill_bridge_url_base(hue_https_handle_t hue_https_handle, char* bridge_ip) {
    if (HUE_NULL_CHECK(tag, hue_https_handle)) return ESP_ERR_INVALID_ARG;

    /* Redundant checking to ensure string has not been deleted before copying */
    if (check_bridge_ip(bridge_ip) != ESP_OK) return ESP_ERR_INVALID_ARG;

    /* Print URL base using set IP address */
    esp_err_t err = snprintf(hue_https_handle->buff_url, HUE_URL_BASE_SIZE, "https://%s" HUE_RESOURCE_PATH, bridge_ip);

    /* Verify that URL base is within expected length */
    if (err < 0) {
        ESP_LOGE(tag, "URL string printing encoding failure");
        return ESP_ERR_INVALID_RESPONSE;
    }
    if (err >= HUE_URL_BASE_SIZE) {
        ESP_LOGE(tag, "URL buffer ran out of characters to print to");
        return ESP_ERR_INVALID_SIZE;
    }

    /* Point to the end of the printed URL base as the point to add the resource path for requests */
    hue_https_handle->url_resource_path = hue_https_handle->buff_url + err;

    return ESP_OK;
}

static esp_err_t check_bridge_id(char* bridge_id) {
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

static esp_err_t fill_bridge_id(hue_https_handle_t hue_https_handle, char* bridge_id) {
    if (HUE_NULL_CHECK(tag, hue_https_handle)) return ESP_ERR_INVALID_ARG;

    /* Redundant checking to ensure string has not been deleted before copying */
    if (check_bridge_id(bridge_id) != ESP_OK) return ESP_ERR_INVALID_ARG;

    /* Copy set Bridge ID to instance buffer */
    strncpy(hue_https_handle->bridge_id, bridge_id, HUE_BRIDGE_ID_LENGTH);

    /* Verify that copied Bridge ID is of the expected format */
    if (check_bridge_id(hue_https_handle->bridge_id) != ESP_OK) return ESP_ERR_INVALID_RESPONSE;

    return ESP_OK;
}

static esp_err_t check_app_key(char* app_key) {
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

static esp_err_t fill_app_key(hue_https_handle_t hue_https_handle, char* app_key) {
    if (HUE_NULL_CHECK(tag, hue_https_handle)) return ESP_ERR_INVALID_ARG;

    /* Redundant checking to ensure string has not been deleted before copying */
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
    if(((*p_hue_https_handle)->handle_evt = xEventGroupCreate()) == NULL){
        ESP_LOGE(tag, "Failed to create Event Group");
        free_hue_https_instance(p_hue_https_handle);
        return ESP_ERR_NO_MEM;
    }
    if(((*p_hue_https_handle)->request_handle_mutex = xSemaphoreCreateMutex()) == NULL){
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

    return ESP_OK;
}