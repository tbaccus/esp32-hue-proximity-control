#ifndef H_WIFI_CONNECT
#define H_WIFI_CONNECT

#include "esp_event.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Modifcation of MAC2STR macro for sscanf */
#define MAC2STR_PTR(a) &(a)[0], &(a)[1], &(a)[2], &(a)[3], &(a)[4], &(a)[5]

#define MACSTR_PARSE "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx"

/* Event base for simplified WiFi connection events */
ESP_EVENT_DECLARE_BASE(WIFI_CONNECT_EVENT);

/** @brief WiFi connect event declarations */
typedef enum {
    WIFI_CONNECT_EVENT_CONNECTED,   /**< WiFi successfully connected, data as esp_netif_ip_info_t */
    WIFI_CONNECT_EVENT_DISCONNECTED /**< WiFi disconnected or failed to connect, data as wifi_err_reason_t */
} wifi_connect_event_t;

/** @brief Advanced WiFi Connect configuration arguments */
typedef struct {
    bool bssid_set;          /**< Enable setting of AP BSSID to connect to */
    char bssid_str[18];      /**< AP BSSID as string */
    bool static_ip_set;      /**< Enable setting of static IP address */
    char ip_str[16];         /**< IP address as string to request from DHCP */
    char gateway_str[16];    /**< Gateway address as string to request from DHCP */
    char netmask_str[16];    /**< Netmask as string to request from DHCP */
    bool timeout_set;        /**< Enable use of WiFi connection timeout */
    uint8_t timeout_seconds; /**< Period of time before timeout is triggered, must be in range [1-10] */
} wifi_connect_advanced_config_t;

/** @brief WiFi Connect configuration arguments */
typedef struct {
    unsigned char ssid[32];     /**< SSID of network to connect to */
    unsigned char password[64]; /**< Password of network to connect to */

    /** Optional advanced arguments for potentially improved WiFi connection stability */
    wifi_connect_advanced_config_t advanced_configs;
} wifi_connect_config_t;

/**
 * @brief Disconnect and deinitialize WiFi
 */
void wifi_disconnect();

/**
 * @brief Connect to WiFi with settings specified in ESP-IDF configs
 *
 * @param[in] wifi_config Pointer to WiFi configuration settings
 *
 * @return ESP Error code
 * @retval - @c ESP_OK – WiFi connection process successfully started
 * @retval - @c ESP_ERR_INVALID_ARG – Configuration argument is NULL
 *
 * @attention \c WIFI_CONNECT_EVENT events will post to the default event loop for WiFi connection and disconnection and
 *            should be registered with esp_event_handler_instance_register to detect and respond to events
 */
esp_err_t wifi_connect(wifi_connect_config_t* wifi_config);

#ifdef __cplusplus
}
#endif

#endif /* H_WIFI_CONNECT */