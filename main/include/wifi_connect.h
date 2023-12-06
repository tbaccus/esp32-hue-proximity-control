#ifndef H_WIFI_CONNECT
#define H_WIFI_CONNECT

#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Optional timeout period for WiFi connection attempt before restarting esp */
#ifdef CONFIG_HUE_WIFI_SET_TIMEOUT
#define WIFI_TIMEOUT_PERIOD pdMS_TO_TICKS(CONFIG_HUE_WIFI_TIMEOUT * 1000)
#endif

/* Event base for simplified WiFi connection events */
ESP_EVENT_DECLARE_BASE(WIFI_CONNECT_EVENT);

/** WiFi connect event declarations */
typedef enum {
    WIFI_CONNECT_EVENT_CONNECTED,       /**< WiFi successfully connected */
    WIFI_CONNECT_EVENT_DISCONNECTED     /**< WiFi disconnected or failed to connect */
} wifi_connect_event_t;

void wifi_connect(void);

#ifdef __cplusplus
}
#endif

#endif /* H_WIFI_CONNECT */