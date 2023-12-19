idf_component_register(SRCS "wifi_connect.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event
                    PRIV_REQUIRES freertos esp_wifi log esp_system esp_common esp_netif esp_hw_support hue_helpers lwip)