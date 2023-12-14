#include "unity.h"
#include "unity_test_runner.h"

#include "hue_json_builder.h"

/*======================= Basic NULL testing =======================*/
TEST_CASE("NULL buffer", "[hue_json_builder][hue_json_light][empty]") {
    hue_light_data_t light;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, hue_light_data_to_json(NULL, &light));
}

TEST_CASE("NULL light data", "[hue_json_builder][hue_json_light][empty]") {
    hue_json_buffer_t buffer;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, hue_light_data_to_json(&buffer, NULL));
}

TEST_CASE("Empty light data", "[hue_json_builder][hue_json_light][empty]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true}}", buffer.buff);
}

/*======================== "on" tag testing ========================*/
TEST_CASE("Set off", "[hue_json_builder][hue_json_light][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.off = true};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":false}}", buffer.buff);
}

/*===================== "dimming" tag testing ======================*/
TEST_CASE("Brightness set no value", "[hue_json_builder][hue_json_light][hue_json_light_range]][empty]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_SET};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming\":{\"brightness\":1}}", buffer.buff);
}

TEST_CASE("Brightness set under range", "[hue_json_builder][hue_json_light][hue_json_light_range]][under_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_SET, .brightness = 0};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming\":{\"brightness\":1}}", buffer.buff);
}

TEST_CASE("Brightness set over range", "[hue_json_builder][hue_json_light][hue_json_light_range]][over_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_SET, .brightness = 127};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming\":{\"brightness\":100}}", buffer.buff);
}

TEST_CASE("Brightness set in range", "[hue_json_builder][hue_json_light][hue_json_light_range]][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_SET, .brightness = 23};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming\":{\"brightness\":23}}", buffer.buff);
}

/*================== "dimming_delta" tag testing ===================*/
TEST_CASE("Brightness add no value", "[hue_json_builder][hue_json_light][hue_json_light_dimming_delta][empty]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_ADD};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming_delta\":{\"action\":\"up\",\"brightness_delta\":0}}",
                             buffer.buff);
}

TEST_CASE("Brightness add under range", "[hue_json_builder][hue_json_light][hue_json_light_dimming_delta][under_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_ADD, .brightness = -1};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming_delta\":{\"action\":\"up\",\"brightness_delta\":100}}",
                             buffer.buff);
}

TEST_CASE("Brightness add over range", "[hue_json_builder][hue_json_light][hue_json_light_dimming_delta][over_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_ADD, .brightness = 127};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming_delta\":{\"action\":\"up\",\"brightness_delta\":100}}",
                             buffer.buff);
}

TEST_CASE("Brightness add in range", "[hue_json_builder][hue_json_light][hue_json_light_dimming_delta][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_ADD, .brightness = 23};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming_delta\":{\"action\":\"up\",\"brightness_delta\":23}}",
                             buffer.buff);
}

TEST_CASE("Brightness subtract no value", "[hue_json_builder][hue_json_light][hue_json_light_dimming_delta][empty]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_SUBTRACT};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming_delta\":{\"action\":\"down\",\"brightness_delta\":0}}",
                             buffer.buff);
}

TEST_CASE("Brightness subtract under range", "[hue_json_builder][hue_json_light][hue_json_light_dimming_delta][under_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_SUBTRACT, .brightness = -1};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming_delta\":{\"action\":\"down\",\"brightness_delta\":100}}",
                             buffer.buff);
}

TEST_CASE("Brightness subtract over range", "[hue_json_builder][hue_json_light][hue_json_light_dimming_delta][over_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_SUBTRACT, .brightness = 127};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming_delta\":{\"action\":\"down\",\"brightness_delta\":100}}",
                             buffer.buff);
}

TEST_CASE("Brightness subtract in range", "[hue_json_builder][hue_json_light][hue_json_light_dimming_delta][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.brightness_action = HUE_ACTION_SUBTRACT, .brightness = 23};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"dimming_delta\":{\"action\":\"down\",\"brightness_delta\":23}}",
                             buffer.buff);
}

/*================ "color_temperature" tag testing =================*/
TEST_CASE("Color Temperature set no value", "[hue_json_builder][hue_json_light][hue_json_light_color_temp][empty]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_SET};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature\":{\"mirek\":153}}", buffer.buff);
}

TEST_CASE("Color Temperature set under range", "[hue_json_builder][hue_json_light][hue_json_light_color_temp][under_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_SET, .color_temp = 0};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature\":{\"mirek\":153}}", buffer.buff);
}

TEST_CASE("Color Temperature set over range", "[hue_json_builder][hue_json_light][hue_json_light_color_temp][over_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_SET, .color_temp = 511};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature\":{\"mirek\":500}}", buffer.buff);
}

TEST_CASE("Color Temperature set in range", "[hue_json_builder][hue_json_light][hue_json_light_color_temp][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_SET, .color_temp = 163};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature\":{\"mirek\":163}}", buffer.buff);
}

/*============= "color_temperature_delta" tag testing ==============*/
TEST_CASE("Color Temperature add no value", "[hue_json_builder][hue_json_light][hue_json_light_color_temp_delta][empty]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_ADD};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature_delta\":{\"action\":\"up\",\"mirek_delta\":0}}",
                             buffer.buff);
}

TEST_CASE("Color Temperature add under range", "[hue_json_builder][hue_json_light][hue_json_light_color_temp_delta][under_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_ADD, .color_temp = -1};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature_delta\":{\"action\":\"up\",\"mirek_delta\":347}}",
                             buffer.buff);
}

TEST_CASE("Color Temperature add over range", "[hue_json_builder][hue_json_light][hue_json_light_color_temp_delta][over_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_ADD, .color_temp = 500};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature_delta\":{\"action\":\"up\",\"mirek_delta\":347}}",
                             buffer.buff);
}

TEST_CASE("Color Temperature add in range", "[hue_json_builder][hue_json_light][hue_json_light_color_temp_delta][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_ADD, .color_temp = 23};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature_delta\":{\"action\":\"up\",\"mirek_delta\":23}}",
                             buffer.buff);
}

TEST_CASE("Color Temperature subtract no value", "[hue_json_builder][hue_json_light][hue_json_light_color_temp_delta][empty]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_SUBTRACT};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature_delta\":{\"action\":\"down\",\"mirek_delta\":0}}",
                             buffer.buff);
}

TEST_CASE("Color Temperature subtract under range", "[hue_json_builder][hue_json_light][hue_json_light_color_temp_delta][under_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_SUBTRACT, .color_temp = -1};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature_delta\":{\"action\":\"down\",\"mirek_delta\":347}}",
                             buffer.buff);
}

TEST_CASE("Color Temperature subtract over range", "[hue_json_builder][hue_json_light][hue_json_light_color_temp_delta][over_range][out_of_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_SUBTRACT, .color_temp = 500};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature_delta\":{\"action\":\"down\",\"mirek_delta\":347}}",
                             buffer.buff);
}

TEST_CASE("Color Temperature subtract in range", "[hue_json_builder][hue_json_light][hue_json_light_color_temp_delta][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.color_temp_action = HUE_ACTION_SUBTRACT, .color_temp = 23};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color_temperature_delta\":{\"action\":\"down\",\"mirek_delta\":23}}",
                             buffer.buff);
}

/*====================== "color" tag testing =======================*/
TEST_CASE("Color set no value xy", "[hue_json_builder][hue_json_light][hue_json_light_color][empty]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.set_color = true};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color\":{\"xy\":{\"x\":0.0000,\"y\":0.0000}}}",
                             buffer.buff);
}

TEST_CASE("Color set only x < 10000", "[hue_json_builder][hue_json_light][hue_json_light_color][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.set_color = true, .color_gamut_x = 102};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color\":{\"xy\":{\"x\":0.0102,\"y\":0.0000}}}",
                             buffer.buff);
}

TEST_CASE("Color set only x > 10000", "[hue_json_builder][hue_json_light][hue_json_light_color][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.set_color = true, .color_gamut_x = 16201};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color\":{\"xy\":{\"x\":1.0,\"y\":0.0000}}}",
                             buffer.buff);
}

TEST_CASE("Color set only y < 10000", "[hue_json_builder][hue_json_light][hue_json_light_color][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.set_color = true, .color_gamut_y = 102};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color\":{\"xy\":{\"x\":0.0000,\"y\":0.0102}}}",
                             buffer.buff);
}

TEST_CASE("Color set only y > 10000", "[hue_json_builder][hue_json_light][hue_json_light_color][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.set_color = true, .color_gamut_y = 16201};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color\":{\"xy\":{\"x\":0.0000,\"y\":1.0}}}",
                             buffer.buff);
}

TEST_CASE("Color set x < 10000, y < 10000", "[hue_json_builder][hue_json_light][hue_json_light_color][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.set_color = true, .color_gamut_x = 102, .color_gamut_y = 130};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color\":{\"xy\":{\"x\":0.0102,\"y\":0.0130}}}",
                             buffer.buff);
}

TEST_CASE("Color set x > 10000, y < 10000", "[hue_json_builder][hue_json_light][hue_json_light_color][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.set_color = true, .color_gamut_x = 16201, .color_gamut_y = 9999};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color\":{\"xy\":{\"x\":1.0,\"y\":0.9999}}}",
                             buffer.buff);
}

TEST_CASE("Color set x < 10000, y > 10000", "[hue_json_builder][hue_json_light][hue_json_light_color][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.set_color = true, .color_gamut_x = 9990, .color_gamut_y = 10000};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color\":{\"xy\":{\"x\":0.9990,\"y\":1.0}}}",
                             buffer.buff);
}

TEST_CASE("Color set x > 10000, y > 10000", "[hue_json_builder][hue_json_light][hue_json_light_color][in_range]") {
    hue_json_buffer_t buffer;
    hue_light_data_t light = {.set_color = true, .color_gamut_x = 10001, .color_gamut_y = 16201};
    TEST_ASSERT_EQUAL(ESP_OK, hue_light_data_to_json(&buffer, &light));
    TEST_ASSERT_EQUAL_STRING("{\"on\":{\"on\":true},\"color\":{\"xy\":{\"x\":1.0,\"y\":1.0}}}",
                             buffer.buff);
}