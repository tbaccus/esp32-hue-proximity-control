idf_component_register(SRCS "main.c"
                    REQUIRES freertos driver nvs_flash esp_phy esp_common wifi_connect hue_json_builder hue_https)