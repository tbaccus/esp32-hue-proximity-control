idf_component_register(SRCS "test.c" "main.c"
                    REQUIRES freertos driver nvs_flash esp_phy esp_common esp_event wifi_connect hue_json_builder hue_https)