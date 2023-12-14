idf_component_register(SRCS "hue_https.c"
                    INCLUDE_DIRS "include"
                    EMBED_TXTFILES hue_signify_root_cert.pem
                    REQUIRES hue_json_builder esp_common
                    PRIV_REQUIRES hue_helpers)