#ifndef H_HUE_HELPERS
#define H_HUE_HELPERS

#include "esp_types.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HUE_BOOL_STR(x) ((x) ? "true" : "false")
#define HUE_NULL_CHECK(tag, x) (x) == NULL){ESP_LOGE(tag, "%s is NULL", #x);}if((x) == NULL

#ifdef __cplusplus
}
#endif

#endif /* H_HUE_HELPERS */