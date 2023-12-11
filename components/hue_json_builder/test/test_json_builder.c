#include "unity.h"
#include "hue_json_builder.h"

TEST_CASE("Basic test", "[light]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {
        .on = true
    };
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true}}", buffer.buff);
}