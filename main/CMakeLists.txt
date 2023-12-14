idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver nvs_flash esp_phy wifi_connect hue_test_app)