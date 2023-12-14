#include "unity.h"
#include "unity_test_runner.h"

#include "hue_json_builder.h"

/*======================= Basic NULL testing =======================*/
TEST_CASE("NULL buffer", "[hue_json_builder][hue_json_smart_scene][empty]") {
    hue_smart_scene_data_t smart_scene;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, hue_smart_scene_data_to_json(NULL, &smart_scene));
}

TEST_CASE("NULL smart scene data", "[hue_json_builder][hue_json_smart_scene][empty]") {
    hue_json_buffer_t buffer;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, hue_smart_scene_data_to_json(&buffer, NULL));
}

TEST_CASE("Empty smart scene data", "[hue_json_builder][hue_json_smart_scene][empty]") {
    hue_json_buffer_t buffer;
    hue_smart_scene_data_t smart_scene = {};
    TEST_ASSERT_EQUAL(ESP_OK, hue_smart_scene_data_to_json(&buffer, &smart_scene));
    TEST_ASSERT_EQUAL_STRING("{\"recall\":{\"action\":\"activate\"}}", buffer.buff);
}

/*====================== "recall" tag testing ======================*/
TEST_CASE("Deactivate", "[hue_json_builder][hue_json_smart_scene][in_range]") {
    hue_json_buffer_t buffer;
    hue_smart_scene_data_t smart_scene = {.deactivate = true};
    TEST_ASSERT_EQUAL(ESP_OK, hue_smart_scene_data_to_json(&buffer, &smart_scene));
    TEST_ASSERT_EQUAL_STRING("{\"recall\":{\"action\":\"deactivate\"}}", buffer.buff);
}