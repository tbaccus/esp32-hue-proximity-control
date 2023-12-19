#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_phy_init.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "wifi_connect.h"
#include "hue_https.h"
#include "hue_json_builder.h"

// #include "hue_test_app.h"

static const char* tag = "main";

static void hue_task(void* pvparameters) {
    hue_grouped_light_data_t grouped_light = {
        .resource_id = CONFIG_HUE_GROUPED_LIGHT_ID,
        .off = true
    };
    
    hue_smart_scene_data_t smart_scene = {
        .resource_id = CONFIG_HUE_SMART_SCENE_ID
    };

    hue_config_t config = {
        .bridge_id = CONFIG_HUE_BRIDGE_ID,
        .application_key = CONFIG_HUE_APP_KEY,
        .retry_attempts = 5
    };
    sscanf(CONFIG_HUE_BRIDGE_IP, "%hhu.%hhu.%hhu.%hhu", &config.bridge_ip[0], &config.bridge_ip[1], &config.bridge_ip[2], &config.bridge_ip[3]);

    hue_grouped_light_https_request(&grouped_light, &config);
    vTaskDelay(pdMS_TO_TICKS(2000));
    hue_smart_scene_https_request(&smart_scene, &config);
    ESP_LOGI(tag, "High water mark: %d", uxTaskGetStackHighWaterMark(NULL));
    vTaskDelete(NULL);
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_CONNECT_EVENT) {
        switch (event_id) {
            case WIFI_CONNECT_EVENT_CONNECTED:
                gpio_set_level(GPIO_NUM_2, 1);
                ESP_LOGI(tag, "WiFi connected to AP");
                xTaskCreate(&hue_task, "hue_task", 8192, NULL, configMAX_PRIORITIES - 8, NULL);
                break;
            case WIFI_CONNECT_EVENT_DISCONNECTED:
                gpio_set_level(GPIO_NUM_2, 0);
                ESP_LOGW(tag, "WiFi disconnected from AP");
                break;
            default:
                ESP_LOGW(tag, "Unknown wifi_connect event");
        }
    }
}

void app_main(void) {
    // run_tests();
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_phy_erase_cal_data_in_nvs());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    gpio_config_t io_conf = {.intr_type = GPIO_INTR_DISABLE,
                             .mode = GPIO_MODE_OUTPUT,
                             .pin_bit_mask = (1ULL << 2),
                             .pull_down_en = 0,
                             .pull_up_en = 0};
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_2, 0);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_CONNECT_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));

    wifi_connect_config_t wifi_config = {
        .ssid = CONFIG_HUE_WIFI_SSID,
        .password = CONFIG_HUE_WIFI_PASSWORD,
        .advanced_configs = {
            .bssid_set = true,
            .bssid_str = CONFIG_HUE_WIFI_BSSID,
            .timeout_set = true,
            .timeout_seconds = CONFIG_HUE_WIFI_TIMEOUT,
            .static_ip_set = true,
            .ip_str = CONFIG_HUE_WIFI_IP,
            .gateway_str = CONFIG_HUE_WIFI_GW,
            .netmask_str = CONFIG_HUE_WIFI_NM
        }
    };

    esp_log_level_set("wifi_connect", ESP_LOG_DEBUG);
    esp_log_level_set("hue_https", ESP_LOG_DEBUG);
    esp_log_level_set("hue_json_builder", ESP_LOG_DEBUG);
    wifi_connect(&wifi_config);
}