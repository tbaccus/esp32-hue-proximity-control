#include "hue_https.h"

static const char* tag = "hue_https";

extern const char hue_signify_root_cert_pem_start[] asm("_binary_hue_signify_root_cert_pem_start");
extern const char hue_signify_root_cert_pem_end[] asm("_binary_hue_signify_root_cert_pem_end");
