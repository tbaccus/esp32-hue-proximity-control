/**
 * @file hue_helpers.h
 * @author Tanner Baccus
 * @date 10 December 2023
 * @brief Commonly used defintions for Hue components
 */

#ifndef H_HUE_HELPERS
#define H_HUE_HELPERS

#include "esp_types.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/*====================================================================================================================*/
/*===================================================== Defines ======================================================*/
/*====================================================================================================================*/

#define HUE_BOOL_STR(x) ((x) ? "true" : "false")
#define HUE_NULL_CHECK(tag, x) unlikely((x) == NULL)){ESP_LOGE(tag, "%s is NULL", #x);}if(unlikely((x) == NULL)

#ifdef __cplusplus
}
#endif
#endif /* H_HUE_HELPERS */