#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_mac.h"

#include "wifi_connect.h"

static const char* tag = "wifi_connect";

/* Event base for simplified WiFi connection events */
ESP_EVENT_DEFINE_BASE(WIFI_CONNECT_EVENT);

/**
 * @brief Converts IP address string to esp_ip4_addr_t value
 *
 * @param[in]  p_ip     IP address as string
 * @param[out] p_output esp_ip4_addr_t for IP to be stored
 *
 * @return ESP error code
 * @retval - @c ESP_OK – Success
 * @retval - @c ESP_FAIL – Failed to parse input string
 * @retval - @c ESP_ERR_INVALID_ARG – p_output and/or p_ip arguments are NULL
 */
static esp_err_t strtoip(const char* p_ip, esp_ip4_addr_t* p_output);

/**
 * @brief Converts MAC address string to array
 *
 * @param[in]  p_mac    MAC address as string
 * @param[out] p_output uint8_t array with 6 elements for MAC address to be stored
 *
 * @return ESP error code
 * @retval - @c ESP_OK – Success
 * @retval - @c ESP_FAIL – Failed to parse input string
 * @retval - @c ESP_ERR_INVALID_ARG – p_output and/or p_mac arguments are NULL
 */
static esp_err_t strtomac(const char* mac, uint8_t* output);

/**
 * @brief Event handler for WiFi and IP events
 *
 * @param[in] arg        Arguments given with event handler register
 * @param[in] event_base Base of event being handled
 * @param[in] event_id   ID of event being handled
 * @param[in] event_data Data sent by event loop
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/**
 * @brief Run WiFi/LwIP initialization phase of WiFi connection
 */
static void wifi_phase_init();

/**
 * @brief Run configuration phase of WiFi connection
 */
static void wifi_phase_config();

#ifdef CONFIG_HUE_WIFI_SET_IP
/**
 * @brief Sets IP for station to request from DHCP server based on ESP-IDF config
 * @param[in,out] sta_netif Pointer to netif instance for station
 */
static void set_static_ip(esp_netif_t* sta_netif);
#endif

#ifdef WIFI_TIMEOUT_PERIOD
/**
 * @brief Callback function for WiFi timeout timer to restart esp on timeout
 *
 * @param timer_handle Handle for timer calling the function
 */
static void wifi_timer_callback(TimerHandle_t timer_handle);
#endif

static TimerHandle_t timer_handle = NULL;                        /**< WiFi timeout timer handle */
static esp_event_handler_instance_t wifi_event_handler_instance; /**< WIFI_EVENT handler instance */
static esp_event_handler_instance_t ip_event_handler_instance;   /**< IP_EVENT handler instance */

/**
 * @brief Converts IP address string to esp_ip4_addr_t value
 *
 * @param[in]  p_ip     IP address as string
 * @param[out] p_output esp_ip4_addr_t for IP to be stored
 *
 * @return ESP error code
 * @retval - @c ESP_OK – Success
 * @retval - @c ESP_FAIL – Failed to parse input string
 * @retval - @c ESP_ERR_INVALID_ARG – p_output and/or p_ip arguments are NULL
 */
static esp_err_t strtoip(const char* p_ip, esp_ip4_addr_t* p_output) {
    /* Validate input pointers are not NULL */
    if (!p_output) return ESP_ERR_INVALID_ARG;
    if (!p_ip) return ESP_ERR_INVALID_ARG;

    /* Using uint32_t output as uint8_t array */
    uint8_t* p_tmp = &(p_output->addr);

    /* Parse IP address string into output with uint8_t array casting */
    if (4 == sscanf(p_ip, "%3hhu.%3hhu.%3hhu.%3hhu", &p_tmp[0], &p_tmp[1], &p_tmp[2], &p_tmp[3])) {
        ESP_LOGD(tag, "strtoip returned: " IPSTR, IP2STR(p_output));
        return ESP_OK;
    } else { /* Fail if sscanf did not recieve string in correct format */
        return ESP_FAIL;
    }
}

/**
 * @brief Converts MAC address string to array
 *
 * @param[in]  p_mac    MAC address as string
 * @param[out] p_output uint8_t array with 6 elements for MAC address to be stored
 *
 * @return ESP error code
 * @retval - @c ESP_OK – Success
 * @retval - @c ESP_FAIL – Failed to parse input string
 * @retval - @c ESP_ERR_INVALID_ARG – p_output and/or p_mac arguments are NULL
 */
static esp_err_t strtomac(const char* p_mac, uint8_t a_output[6]) {
    /* Validate input pointers are not NULL */
    if (!a_output) return ESP_ERR_INVALID_ARG;
    if (!p_mac) return ESP_ERR_INVALID_ARG;

    /* Parse MAC address string into output array */
    if (6 == sscanf(p_mac, MACSTR, MAC2STR_PTR(a_output))) {
        ESP_LOGD(tag, "strtomac returned: " MACSTR, MAC2STR(a_output));
        return ESP_OK;
    } else { /* Fail if sscanf did not recieve string in correct format */
        return ESP_FAIL;
    }
}

/**
 * @brief Event handler for WiFi and IP events
 *
 * @param[in] arg        Arguments given with event handler register
 * @param[in] event_base Base of event being handled
 * @param[in] event_id   ID of event being handled
 * @param[in] event_data Data sent by event loop
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) { /* WiFi Event handlers */
        switch (event_id) {
            case WIFI_EVENT_STA_START: /* WiFi event for starting connection attempt */
                /* If WiFi timeout is enabled, start timer for connection */
                if (timer_handle) xTimerStart(timer_handle, 0);
                ESP_LOGD(tag, "Starting WiFi Phase 4: Connect");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED: /* WiFi event for connection failure */
                /* Cast event_data for WiFi disconnect specific data */
                wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*)event_data;

                /* Post disconnect event for wifi_connect event handling with disconnect reason code */
                ESP_ERROR_CHECK(esp_event_post(WIFI_CONNECT_EVENT, WIFI_CONNECT_EVENT_DISCONNECTED, &(event->reason),
                                               sizeof(event->reason), portMAX_DELAY));
                ESP_LOGI(tag, "Failed to connect to AP, Reason: %d", event->reason);

                /* If WiFi timeout is enabled, start timer for connection */
                if (timer_handle) xTimerStart(timer_handle, 0);
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_CONNECTED: /* WiFi event for connection success */
                /* If WiFi timeout is enabled, stop timer to prevent esp restart */
                if (timer_handle) xTimerStop(timer_handle, 0);
                ESP_LOGI(tag, "AP connected successfully, requesting IP...");
                ESP_LOGD(tag, "Starting WiFi Phase 5: 'Got IP'");
                break;
            case WIFI_EVENT_STA_STOP: /* WiFi event for connection stop */
                ESP_LOGI(tag, "WiFi connection stopped");
                break;
            case WIFI_EVENT_STA_BEACON_TIMEOUT: /* WiFi event for beacon timeout */
                ESP_LOGI(tag, "WiFi beacon timeout");
                break;
            default: /* WiFi events not accounted for */
                ESP_LOGI(tag, "Unexpected WiFi Event ID: %ld", event_id);
        };
    } else if (event_base == IP_EVENT) { /* IP Event handlers */
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: /* IP event for IP assigned */
                /* Cast event_data for IP assignment specific data */
                ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;

                /* If IP changed, post disconnect event for wifi_connect event handling to restart all connections */
                if (event->ip_changed) {
                    wifi_err_reason_t reason = WIFI_REASON_UNSPECIFIED;
                    ESP_ERROR_CHECK(esp_event_post(WIFI_CONNECT_EVENT, WIFI_CONNECT_EVENT_DISCONNECTED, &reason,
                                                   sizeof(reason), portMAX_DELAY));
                }

                /* Post connect event for wifi_connect event handling with IP info */
                ESP_ERROR_CHECK(esp_event_post(WIFI_CONNECT_EVENT, WIFI_CONNECT_EVENT_CONNECTED, &(event->ip_info),
                                               sizeof(event->ip_info), portMAX_DELAY));
                ESP_LOGI(tag, "Got ip: " IPSTR, IP2STR(&event->ip_info.ip));
                break;
            default: /* IP events not accounted for */
                ESP_LOGI(tag, "Unexpected IP Event ID: %ld", event_id);
        };
    } else { /* Unexpected Event handler */
        ESP_LOGI(tag, "Unexpected Event base: %s, ID: %ld", event_base, event_id);
    }
}

#ifdef WIFI_TIMEOUT_PERIOD
/**
 * @brief Callback function for WiFi timeout timer to restart esp on timeout
 *
 * @param timer_handle Handle for timer calling the function
 */
static void wifi_timer_callback(TimerHandle_t timer_handle) {
    ESP_LOGW(tag, "WiFi connection timed out, restarting to refresh connection...");
    esp_restart();
}
#endif

#ifdef CONFIG_HUE_WIFI_SET_IP
/**
 * @brief Sets IP for station to request from DHCP server based on ESP-IDF config
 * @param[in,out] sta_netif Pointer to netif instance for station
 */
static void set_static_ip(esp_netif_t* sta_netif) {
    if (!sta_netif) return;          /* Stop if netif instance does not exist */
    esp_netif_ip_info_t info = {0};  /* Create empty IP info storage */
    esp_netif_dhcpc_stop(sta_netif); /* Stops DHCP client if running */

    /* Set IP info from ESP-IDF configs */
    ESP_ERROR_CHECK(strtoip(CONFIG_HUE_WIFI_IP, info.ip.addr));
    ESP_ERROR_CHECK(strtoip(CONFIG_HUE_WIFI_GW, info.gw.addr));
    ESP_ERROR_CHECK(strtoip(CONFIG_HUE_WIFI_NM, info.netmask.addr));
    esp_netif_set_ip_info(sta_netif, &info);
}
#endif

/**
 * @brief Run WiFi/LwIP initialization phase of WiFi connection
 */
static void wifi_phase_init() {
    /* Step 1.1: initialize netif */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Step 1.2: initialize event loop */
    /*   Register WiFi and IP events with event_handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL,
                                                        &wifi_event_handler_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL,
                                                        &ip_event_handler_instance));

    /* Step 1.3: create default network interface instance */
#ifdef CONFIG_HUE_WIFI_SET_IP
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    set_static_ip(sta_netif);
#else
    esp_netif_create_default_wifi_sta();
#endif

    /* Step 1.4: create WiFi driver task and initialize driver with esp_wifi_init */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

/**
 * @brief Run configuration phase of WiFi connection
 */
static void wifi_phase_config() {
    /* WiFi configuration with SSID and Password provided in ESP-IDF config */
    static wifi_config_t wifi_config = {.sta = {.ssid = CONFIG_HUE_WIFI_SSID,
                                                .password = CONFIG_HUE_WIFI_PASSWORD,
                                                .scan_method = WIFI_FAST_SCAN,
                                                .threshold.authmode = WIFI_AUTH_WPA_PSK,
                                                .threshold.rssi = -90,
                                                .bssid_set = CONFIG_HUE_WIFI_SET_BSSID}};

    /* Set BSSID of specific AP to connect to if enabled in ESP-IDF config       *
     * Intended to speed up connection if other APs are known to fail more often */
#ifdef CONFIG_HUE_WIFI_SET_BSSID
    ESP_ERROR_CHECK(strtomac(CONFIG_HUE_WIFI_BSSID, wifi_config.sta.bssid));
#endif

    /* Set WiFi to station mode and apply configuration */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    /* TODO: Test if actually helps with connection stability */
    // ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}

/**
 * @brief Disconnect and deinitialize WiFi
 */
void wifi_disconnect() {
    ESP_LOGI(tag, "Disconnecting WiFi...");
    /* Unregister event handler to prevent reconnect attempts during disconnect process */
    if (wifi_event_handler_instance) {
        ESP_LOGD(tag, "Unregistering WiFi event handler instance...");
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler_instance);
    }

    if (ip_event_handler_instance) {
        ESP_LOGD(tag, "Unregistering IP event handler instance...");
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler_instance);
    }

    /* If enabled, delete WiFi timeout timer task */
    if (timer_handle) {
        ESP_LOGD(tag, "Deleting WiFi timer...");
        xTimerDelete(timer_handle, 0);
        timer_handle = NULL;
    }

    /* Post WiFi disconnected event for wifi_connect event handling */
    ESP_LOGD(tag, "Posting WIFI_CONNECT_EVENT_DISCONNECTED...");
    wifi_err_reason_t reason = WIFI_REASON_ASSOC_LEAVE; /* WiFi reason indicating esp_wifi_disconnect() call */
    ESP_ERROR_CHECK(
        esp_event_post(WIFI_CONNECT_EVENT, WIFI_CONNECT_EVENT_DISCONNECTED, &reason, sizeof(reason), portMAX_DELAY));

    /* No error handling needed for WiFi disconnect and deinitialization */
    ESP_LOGD(tag, "Calling esp_wifi_disconnect()...");
    esp_wifi_disconnect();

    ESP_LOGD(tag, "Calling esp_wifi_stop()...");
    esp_wifi_stop();

    ESP_LOGD(tag, "Calling esp_wifi_deinit()...");
    esp_wifi_deinit();

    ESP_LOGI(tag, "WiFi disconnected.");
}

/**
 * @brief Connect to WiFi with settings specified in ESP-IDF configs
 *
 * @attention \c WIFI_CONNECT_EVENT events will post to the default event loop for WiFi connection and disconnection and
 *            should be registered with esp_event_handler_instance_register to detect and respond to events
 */
void wifi_connect() {
    ESP_LOGI(tag, "WiFi connection process started");
#ifdef WIFI_TIMEOUT_PERIOD
    /* WiFi timeout timer for restarting esp if timeout period has passed during connection */
    timer_handle = xTimerCreate("WiFi timer", WIFI_TIMEOUT_PERIOD, pdFALSE, (void*)0, wifi_timer_callback);
#endif

    ESP_LOGD(tag, "Starting WiFi Phase 1: Initialization");
    wifi_phase_init(); /* Phase 1 of WiFi connection */
    ESP_LOGD(tag, "Starting WiFi Phase 2: Configuration");
    wifi_phase_config(); /* Phase 2 of WiFi connection */
    ESP_LOGD(tag, "Starting WiFi Phase 3: Start");
    ESP_ERROR_CHECK(esp_wifi_start()); /* Phase 3 of WiFi connection */
    /* Phase 3 posts WIFI_EVENT_STA_START to event_handler to begin Phase 4 */

    /* Phase 4 of WiFi connection starts with esp_wifi_connect() call in event_handler */
    /* Phase 4 posts WiFI_EVENT_STA_CONNECTED to event_handler to begin Phase 5 */

    /* Phase 5 of WiFi connection is IP assignment from DHCP server */
    /* Phase 5 posts IP_EVENT_STA_GOT_IP to event_handler once IP is assigned */
}