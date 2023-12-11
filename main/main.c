#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_phy_init.h"

#include "wifi_connect.h"
#include "hue_helpers.h"
#include "hue_json_builder.h"

static const char* tag = "main";

// static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
//     if (event_base == WIFI_CONNECT_EVENT) {
//         switch (event_id) {
//             case WIFI_CONNECT_EVENT_CONNECTED:
//                 gpio_set_level(GPIO_NUM_2, 1);
//                 ESP_LOGI(tag, "WiFi connected to AP");
//                 break;
//             case WIFI_CONNECT_EVENT_DISCONNECTED:
//                 gpio_set_level(GPIO_NUM_2, 0);
//                 ESP_LOGW(tag, "WiFi disconnected from AP");
//                 break;
//             default:
//                 ESP_LOGW(tag, "Unknown wifi_connect event");
//         }
//     }
// }

void app_main(void) {
    ESP_LOGI(tag, "app_main");
    // ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(esp_phy_erase_cal_data_in_nvs());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    // gpio_config_t io_conf = {.intr_type = GPIO_INTR_DISABLE,
    //                          .mode = GPIO_MODE_OUTPUT,
    //                          .pin_bit_mask = (1ULL << 2),
    //                          .pull_down_en = 0,
    //                          .pull_up_en = 0};
    // gpio_config(&io_conf);
    // gpio_set_level(GPIO_NUM_2, 0);

    // esp_log_level_set("wifi_connect", ESP_LOG_DEBUG);

    // ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_CONNECT_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));

    // wifi_connect();
}