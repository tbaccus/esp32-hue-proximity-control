#ifndef H_WIFI_CONNECT
#define H_WIFI_CONNECT

#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_HUE_WIFI_SET_TIMEOUT
    /* Optional timeout period for WiFi connection attempt before restarting esp   *
     * If timeout is larger than 10 seconds, it will never time out                *
     * If timeout is less than 1 second, esp will restart on every connect attempt */
    #if (CONFIG_HUE_WIFI_TIMEOUT >= 1) && (CONFIG_HUE_WIFI_TIMEOUT <= 10)
        #define WIFI_TIMEOUT_PERIOD pdMS_TO_TICKS(CONFIG_HUE_WIFI_TIMEOUT * 1000)
    #endif
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

/**
 * @brief Disconnect and deinitialize WiFi
 */
void wifi_disconnect();

/**
 * @brief Connect to WiFi with settings specified in ESP-IDF configs
 *
 * @attention \c WIFI_CONNECT_EVENT events will post to the default event loop for WiFi connection and disconnection and
 *            should be registered with esp_event_handler_instance_register to detect and respond to events
 */
void wifi_connect();

#ifdef __cplusplus
}
#endif

#endif /* H_WIFI_CONNECT */