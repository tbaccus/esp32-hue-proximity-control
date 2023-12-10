#ifndef H_HUE_HELPERS
#define H_HUE_HELPERS

#include <stdint.h>
#include <stdbool.h>
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HUE_BOOL_STR(x) ((x) ? "true" : "false")

bool hue_null_check(const char* tag, void* variable, const char* variable_name);

#define HUE_NULL_CHECK(tag, x) hue_null_check(tag, x, #x)

#ifdef __cplusplus
}
#endif

#endif /* H_HUE_HELPERS */