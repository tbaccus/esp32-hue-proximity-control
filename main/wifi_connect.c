#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_netif.h"

#include "wifi_connect.h"

static const char* tag = "wifi_connect";

/* Event base for simplified WiFi connection events */
ESP_EVENT_DEFINE_BASE(WIFI_CONNECT_EVENT);

static esp_err_t strtomac(const char* mac, uint8_t* output);
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifi_timer_callback(TimerHandle_t timer_handle);
static void set_static_ip(esp_netif_t* sta_netif);
static void wifi_phase_init();
static void wifi_phase_config();
static void wifi_phase_start();

static TimerHandle_t timer_handle;

/**
 * @brief Converts IP address string to esp_ip4_addr_t value
 *
 * @param[in]  ip    IP address as string ()
 * @param[out] output esp_ip4_addr_t for IP to be stored
 *
 * @return ESP error code
 * @retval - @c ESP_OK – Success
 * @retval - @c ESP_FAIL – Failed to parse input string
 * @retval - @c ESP_ERR_INVALID_ARG – output and/or ip arguments are NULL
 */
static esp_err_t strtoip(const char* ip, esp_ip4_addr_t* output) {
    if (!output) return ESP_ERR_INVALID_ARG;
    if (!ip) return ESP_ERR_INVALID_ARG;
    uint8_t tmp[4];
    if (4 == sscanf(ip, "%3hhx.%3hhx.%3hhx.%3hhx", &tmp[0], &tmp[1], &tmp[2], &tmp[3])) {
        ESP_LOGD(tag, "strtoip returned: %d.%d.%d.%d", tmp[0], tmp[1], tmp[2], tmp[3]);
        output->addr = tmp[0] | (tmp[1] << 010) | (tmp[2] << 020) | (tmp[3] << 030);
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

/**
 * @brief Converts MAC address string to array
 *
 * @param[in]  mac    MAC address as string
 * @param[out] output uint8_t array with 6 elements for MAC address to be stored
 *
 * @return ESP error code
 * @retval - @c ESP_OK – Success
 * @retval - @c ESP_FAIL – Failed to parse input string
 * @retval - @c ESP_ERR_INVALID_ARG – output and/or mac arguments are NULL
 */
static esp_err_t strtomac(const char* mac, uint8_t* output) {
    if (!output) return ESP_ERR_INVALID_ARG;
    if (!mac) return ESP_ERR_INVALID_ARG;
    if (6 == sscanf(mac, "%2hhx%*1[-:]%2hhx%*1[-:]%2hhx%*1[-:]%2hhx%*1[-:]%2hhx%*1[-:]%2hhx", &output[0], &output[1],
                    &output[2], &output[3], &output[4], &output[5])) {
        ESP_LOGD(tag, "strtomac returned: %hhx:%hhx:%hhx:%hhx:%hhx:%hhx", output[0], output[1], output[2], output[3],
                 output[4], output[5]);
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if (timer_handle) xTimerStart(timer_handle, 0);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*)event_data;
        ESP_ERROR_CHECK(esp_event_post(WIFI_CONNECT_EVENT, WIFI_CONNECT_EVENT_DISCONNECTED, &(event->reason),
                                       sizeof(event->reason), portMAX_DELAY));
        if (event->reason == WIFI_REASON_ASSOC_LEAVE) return;
        if (event->reason == WIFI_REASON_AUTH_LEAVE) return;
        ESP_LOGW(tag, "Failed to connect to AP, Reason: %d", event->reason);

        if (timer_handle) xTimerStart(timer_handle, 0);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        if (timer_handle) xTimerStop(timer_handle, 0);
        ESP_LOGI(tag, "AP connected successfully");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_ERROR_CHECK(esp_event_post(WIFI_CONNECT_EVENT, WIFI_CONNECT_EVENT_CONNECTED, &(event->ip_info),
                                       sizeof(event->ip_info), portMAX_DELAY));
        ESP_LOGI(tag, "Got ip: " IPSTR, IP2STR(&event->ip_info.ip));
    } else {
        ESP_LOGW(tag, "WiFi Event: %s: %ld", event_base, event_id);
    }
}

static void wifi_timer_callback(TimerHandle_t timer_handle) {
    ESP_LOGW(tag, "WiFi connection timed out, restarting to refresh connection...");
    esp_restart();
}

#ifdef CONFIG_HUE_WIFI_SET_IP
static void set_static_ip(esp_netif_t* sta_netif) {
    if (!sta_netif) return;
    esp_netif_ip_info_t info = {0};
    esp_netif_dhcpc_stop(sta_netif);
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
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

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

static void wifi_phase_config() {
    static wifi_config_t wifi_config = {.sta = {.ssid = CONFIG_HUE_WIFI_SSID,
                                                .password = CONFIG_HUE_WIFI_PASSWORD,
                                                .scan_method = WIFI_FAST_SCAN,
                                                .threshold.authmode = WIFI_AUTH_WPA_PSK,
                                                .threshold.rssi = -90,
                                                .bssid_set = CONFIG_HUE_WIFI_SET_BSSID}};

#ifdef CONFIG_HUE_WIFI_SET_BSSID
    ESP_ERROR_CHECK(strtomac(CONFIG_HUE_WIFI_BSSID, wifi_config.sta.bssid));
#endif

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    // ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}

static void wifi_phase_start() { ESP_ERROR_CHECK(esp_wifi_start()); }

void wifi_init_sta(void) {
    timer_handle = xTimerCreate("WiFi timer", WIFI_TIMEOUT_PERIOD, pdFALSE, (void*)0, wifi_timer_callback);

    wifi_phase_init();
    wifi_phase_config();
    wifi_phase_start();

    ESP_LOGI(tag, "wifi_init_sta finished.");
}