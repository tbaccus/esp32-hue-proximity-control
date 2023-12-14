idf_component_register(SRCS "hue_json_builder.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_common
                    PRIV_REQUIRES hue_helpers log)