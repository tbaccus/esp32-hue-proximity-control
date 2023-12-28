#include <stdlib.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_phy_init.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "wifi_connect.h"
#include "hue_https.h"
#include "hue_json_builder.h"

// #include "hue_test_app.h"

// #include "test.h"

static const char* tag = "main";

// static TaskHandle_t hue_task_handle;

// static void hue_task(void* pvparameters) {
//     hue_grouped_light_data_t grouped_light = {
//         .resource_id = CONFIG_HUE_GROUPED_LIGHT_ID,
//         .off = true
//     };
//     hue_smart_scene_data_t smart_scene = {
//         .resource_id = CONFIG_HUE_SMART_SCENE_ID
//     };
//     hue_config_t config = {
//         .bridge_id = CONFIG_HUE_BRIDGE_ID,
//         .application_key = CONFIG_HUE_APP_KEY,
//         .retry_attempts = 5
//     };
//     sscanf(CONFIG_HUE_BRIDGE_IP, "%hhu.%hhu.%hhu.%hhu", &config.bridge_ip[0], &config.bridge_ip[1], &config.bridge_ip[2], &config.bridge_ip[3]);
//     hue_grouped_light_https_request(&grouped_light, &config);
//     vTaskDelay(pdMS_TO_TICKS(2000));
//     hue_smart_scene_https_request(&smart_scene, &config);
//     ESP_LOGI(tag, "High water mark: %d", uxTaskGetStackHighWaterMark(NULL));
//     wifi_disconnect();
//     vTaskDelete(NULL);
// }

static hue_https_handle_t hue_handle;
static hue_https_request_handle_t on_handle;
static hue_https_request_handle_t off_handle;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_CONNECT_EVENT) {
        switch (event_id) {
            case WIFI_CONNECT_EVENT_CONNECTED:
                gpio_set_level(GPIO_NUM_2, 1);
                ESP_LOGI(tag, "WiFi connected to AP");
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

void run(void) {
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

    // xTaskCreate(&hue_task, "hue_task", 8192, NULL, configMAX_PRIORITIES - 8, &hue_task_handle);
    

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

    hue_https_config_t hue_config = {
        .application_key = CONFIG_HUE_APP_KEY,
        .bridge_id = CONFIG_HUE_BRIDGE_ID,
        .bridge_ip = CONFIG_HUE_BRIDGE_IP,
        .retry_attempts = 5,
        .task_id = "hue_https"
    };
    
    hue_https_create_instance(&hue_handle, &hue_config);

    hue_grouped_light_data_t on_data = {
        .resource_id = CONFIG_HUE_GROUPED_LIGHT_ID
    };
    hue_https_create_grouped_light_request(&on_handle, &on_data);

    hue_smart_scene_data_t off_data = {
        .resource_id = CONFIG_HUE_SMART_SCENE_ID,
        .deactivate = true
    };
    hue_https_create_smart_scene_request(&off_handle, &off_data);

    // wifi_connect(&wifi_config);
}

// #define CONNECTED_BIT BIT0
// #define TOGGLE_BIT BIT1
// #define ON_BIT BIT2
// #define LIGHT_BIT BIT3

// #define RNUM(min,max) ((min) + (rand() % ((max) - (min) + 1)))
// #define BIT_FORMAT "%s%s%s%s"
// #define CBIT(val) ((val & CONNECTED_BIT) ? (LOG_COLOR(LOG_COLOR_GREEN) "C1 " LOG_RESET_COLOR) : (LOG_COLOR(LOG_COLOR_RED) "C0 " LOG_RESET_COLOR))
// #define TBIT(val) ((val & TOGGLE_BIT) ? (LOG_COLOR(LOG_COLOR_BROWN) "T1 " LOG_RESET_COLOR) : (LOG_COLOR(LOG_COLOR_BLACK) "T0 " LOG_RESET_COLOR))
// #define OBIT(val) ((val & ON_BIT) ? (LOG_COLOR(LOG_COLOR_BROWN) "O1 " LOG_RESET_COLOR) : (LOG_COLOR(LOG_COLOR_BLACK) "O0 " LOG_RESET_COLOR))
// #define LBIT(val) ((val & LIGHT_BIT) ? (LOG_COLOR(LOG_COLOR_BROWN) " @ " LOG_RESET_COLOR) : (LOG_COLOR(LOG_COLOR_BLACK) " @ " LOG_RESET_COLOR))
// #define BITS(val) CBIT(val), TBIT(val), OBIT(val), LBIT(val)

// static TaskHandle_t task_handle;
// static TaskHandle_t connection_task_handle;
// static TaskHandle_t toggle_task_handle;
// static EventGroupHandle_t evt;

// static void fun(uint8_t on_bit, size_t times);
// static void task(void* pvparameters);
// static void connection_task(void* pvparameters);
// static void toggle_task(void* pvparameters);

// static void fun(uint8_t on_bit, size_t times) {
//     uint32_t notif_val = 0;
//     for(int i = 1; i <= times; i++) {
//         notif_val = xEventGroupGetBits(evt);
//         if(!(notif_val & CONNECTED_BIT)) {
//             return;
//         }
//         if(on_bit != (notif_val & ON_BIT)) return;
//         printf("\t\t" BIT_FORMAT "\n", BITS(xEventGroupGetBits(evt)));
//         vTaskDelay(pdMS_TO_TICKS(RNUM(1, 10) * 200));
//     }
//     xEventGroupClearBits(evt, TOGGLE_BIT);
//     if(on_bit) {
//         xEventGroupSetBits(evt, LIGHT_BIT);
//     } else {
//         xEventGroupClearBits(evt, LIGHT_BIT);
//     }
//     printf("Clear Toggle\t" BIT_FORMAT "\n", BITS(xEventGroupGetBits(evt)));
// }

// static void task(void* pvparameters) {
//     vTaskSuspend(NULL);
//     uint8_t notif_val = 0;
//     while(true) {
//         notif_val = xEventGroupWaitBits(evt, CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
//         while(true) {
//             notif_val = xEventGroupWaitBits(evt, TOGGLE_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
//             if(!(notif_val & CONNECTED_BIT)) break;
//             fun(notif_val & ON_BIT, RNUM(1, 5));
//         }
//     }
//     vTaskDelete(NULL);
// }

// static void connection_task(void* pvparameters) {
//     vTaskDelay(pdMS_TO_TICKS(500));
//     vTaskResume(task_handle);
//     vTaskResume(toggle_task_handle);
//     while(true) {
//         xEventGroupSetBits(evt, CONNECTED_BIT);
//         printf("Connected\t" BIT_FORMAT "\n", BITS(xEventGroupGetBits(evt)));
//         vTaskDelay(pdMS_TO_TICKS(RNUM(10, 20) * 500));
//         xEventGroupClearBits(evt, CONNECTED_BIT);
//         printf("Disconnected\t" BIT_FORMAT "\n", BITS(xEventGroupGetBits(evt)));
//         vTaskDelay(pdMS_TO_TICKS(RNUM(2, 10) * 500));
//     }
//     vTaskDelete(NULL);
// }

// static void toggle_task(void* pvparameters) {
//     vTaskSuspend(NULL);
//     uint8_t type = 0;
//     while(true) {
//         type = rand() % 3;
//         if(type == 1) {
//             xEventGroupSetBits(evt, TOGGLE_BIT | ON_BIT);
//             printf("Toggle On\t" BIT_FORMAT "\n", BITS(xEventGroupGetBits(evt)));
//         } else if(type == 2){
//             xEventGroupClearBits(evt, ON_BIT);
//             xEventGroupSetBits(evt, TOGGLE_BIT);
//             printf("Toggle Off\t" BIT_FORMAT "\n", BITS(xEventGroupGetBits(evt)));
//         } else {
//             printf("No Change\t" BIT_FORMAT "\n", BITS(xEventGroupGetBits(evt)));
//         }
//         vTaskDelay(pdMS_TO_TICKS((10 + (rand() % 20)) * 500));
//     }
//     vTaskDelete(NULL);
// }

// static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
//     switch((test_event_t) event_id) {
//         case TEST_EVENT_ON:
//             printf(LOG_COLOR(LOG_COLOR_GREEN) "TEST_EVENT_ON\n" LOG_RESET_COLOR);
//             while(true) {
//                 vTaskDelay(pdMS_TO_TICKS(200));
//             }
//             break;
//         case TEST_EVENT_OFF:
//             printf(LOG_COLOR(LOG_COLOR_RED) "TEST_EVENT_OFF\n" LOG_RESET_COLOR);
//             break;
//         default:
//     }
// }


// void testing(void) {
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     create_test_task();
//     ESP_ERROR_CHECK(esp_event_handler_instance_register(TEST_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
//     // if((evt = xEventGroupCreate()) == NULL) return;
//     // xTaskCreate(&task, "task", 4096, NULL, configMAX_PRIORITIES - 5, &task_handle);
//     // xTaskCreate(&toggle_task, "toggle_task", 4096, NULL, configMAX_PRIORITIES - 4, &toggle_task_handle);
//     // xTaskCreate(&connection_task, "connection_task", 4096, NULL, configMAX_PRIORITIES - 3, &connection_task_handle);
// }

void app_main(void) {
    // run_tests();
    srand(time(NULL));
    run();
}