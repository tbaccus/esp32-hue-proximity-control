idf_component_register(SRCS "hue_https_request_instance.c" "hue_https_instance.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "private_include"
                    EMBED_TXTFILES hue_signify_root_cert.pem
                    REQUIRES hue_json_builder esp_common
                    PRIV_REQUIRES hue_helpers esp_http_client esp-tls log freertos esp_wifi)