idf_component_register(SRCS "hue_https.c" "wifi_connect.c" "main.c"
                    INCLUDE_DIRS "include"
                    EMBED_TXTFILES hue_signify_root_cert.pem)