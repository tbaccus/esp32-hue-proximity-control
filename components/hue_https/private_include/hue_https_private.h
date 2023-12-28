/**
 * @file hue_https_private.h
 * @author Tanner Baccus
 * @date 26 December 2023
 * @brief Declarations of all structures and functions shared between component modules but private to component use
 */

#ifndef H_HUE_HTTPS_PRIVATE
#define H_HUE_HTTPS_PRIVATE

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_http_client.h"
#include "esp_bit_defs.h"

#include "hue_https.h"

#ifdef __cplusplus
extern "C" {
#endif

/*====================================================================================================================*/
/*===================================================== Defines ======================================================*/
/*====================================================================================================================*/

#define HUE_REQUEST_BUFFER_SIZE 512

/** Bridge ID scanf format with 16 hexadecimal characters */
#define HUE_BRIDGE_ID_FORMAT "%*16x"
/** Length of bridge ID without null-terminating character */
#define HUE_BRIDGE_ID_LENGTH 16

/** Bridge IP scanf format with standard 4 byte IP formatting */
#define HUE_BRIDGE_IP_FORMAT "%*3u.%*3u.%*3u.%*3u"
/** Length of bridge IP without null-terminating character */
#define HUE_BRIDGE_IP_LENGTH 15

/** Application key scanf format with 40 URL Base64 characters */
#define HUE_APPLICATION_KEY_FORMAT "%*40[-_0-9a-zA-Z]"
/** Length of application key without null-terminating character */
#define HUE_APPLICATION_KEY_LENGTH 40

/** Resource ID scanf format using hexadecimal characters: "[8 chars]-[4 chars]-[4 chars]-[4 chars]-[12 chars]" */
#define HUE_RESOURCE_ID_FORMAT "%*8x-%*4x-%*4x-%*4x-%*12x"
/** Length of resource ID format without null-terminating character */
#define HUE_RESOURCE_ID_LENGTH 36

/** Philips Hue path to resource */
#define HUE_RESOURCE_PATH "/clip/v2/resource/"
/** Length of HUE_RESOURCE_PATH without null-terminating character */
#define HUE_RESOURCE_PATH_LENGTH 18

/** Length of "https://" + strlen("0.0.0.0") + HUE_RESOURCE_PATH */
#define HUE_URL_BASE_MIN_LENGTH 8 + 7 + HUE_RESOURCE_PATH_LENGTH
/** Length of "https://" + strlen("000.000.000.000") + HUE_RESOURCE_PATH */
#define HUE_URL_BASE_MAX_LENGTH 8 + 15 + HUE_RESOURCE_PATH_LENGTH
/** Size of "https://" + IPV4 address + HUE_RESOURCE_PATH */
#define HUE_URL_BASE_SIZE HUE_URL_BASE_MAX_LENGTH + 1
/** Length of longest resource type id + resource id length */
#define HUE_URL_RES_PATH_LENGTH HUE_RESOURCE_TYPE_SIZE + HUE_RESOURCE_ID_LENGTH
/** Size of "https://" + IPV4 address + HUE_RESOURCE_PATH + longest resource type id + resource id length */
#define HUE_URL_BUFFER_SIZE HUE_URL_BASE_SIZE + HUE_URL_RES_PATH_LENGTH

#define HUE_HTTPS_EVT_WIFI_CONNECTED_BIT BIT0
#define HUE_HTTPS_EVT_TRIGGER_BIT BIT1
#define HUE_HTTPS_EVT_ABORT_BIT BIT2
#define HUE_HTTPS_EVT_EXIT_BIT BIT3

#define HUE_HTTPS_EVT_WAIT_BITS HUE_HTTPS_EVT_WIFI_CONNECTED_BIT | HUE_HTTPS_EVT_TRIGGER_BIT | HUE_HTTPS_EVT_EXIT_BIT

/*====================================================================================================================*/
/*======================================= Shared Private Structure Definitions =======================================*/
/*====================================================================================================================*/

/** @brief Storage for all required data for hue_https instance */
typedef struct hue_https_instance {
    TaskHandle_t task_handle;      /**< Task handle for performing requests with instance */
    EventGroupHandle_t handle_evt; /**< Event group for communication from request instances to https instance */

    char buff_url[HUE_URL_BUFFER_SIZE];           /**< Buffer for request URL */
    uint8_t url_res_path_pos;                     /**< Pointer to URL buffer where resource path will fill */
    char bridge_id[HUE_BRIDGE_ID_LENGTH + 1];     /**< Bridge ID needed for CA Cert verification*/
    char app_key[HUE_APPLICATION_KEY_LENGTH + 1]; /**< Application key needed for requests */
    esp_http_client_config_t client_config;       /**< Config for http clients under this instance */

    SemaphoreHandle_t request_handle_mutex;            /**< Protects request handles from parallel tasks */
    hue_https_request_handle_t current_request_handle; /**< Handle for request being performed */
    hue_https_request_handle_t next_request_handle;    /**< Handle for request to replace current */
    uint8_t retry_attempts; /**< Maximum number of times to retry HTTPS request before failing */
} hue_https_instance_t;

/** @brief Storage for HTTP request body and URL resource path */
typedef struct hue_https_request_instance {
    char* request_body;  /**< Body of HTTP request storing Philips Hue actions */
    char* resource_path; /**< URL path to resource with resource type and ID */
} hue_https_request_instance_t;

/*====================================================================================================================*/
/*======================================= Shared Private Function Declarations =======================================*/
/*====================================================================================================================*/

#ifdef __cplusplus
}
#endif
#endif /* H_HUE_HTTPS_PRIVATE */