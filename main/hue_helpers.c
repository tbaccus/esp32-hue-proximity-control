#include "hue_helpers.h"

bool hue_null_check(const char* tag, void* variable, const char* variable_name) {
    if(!variable) {
        ESP_LOGE(tag, "%s is NULL", variable_name);
        return false;
    }
    return true;
}