idf_component_register(SRCS "hue_json_builder.c" "hue_helpers.c" "hue_https.c" "wifi_connect.c" "main.c"
                    INCLUDE_DIRS "include"
                    EMBED_TXTFILES hue_signify_root_cert.pem)