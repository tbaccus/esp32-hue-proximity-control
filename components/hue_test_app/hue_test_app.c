#include <stdio.h>
#include "unity.h"

#include "hue_test_app.h"

void run_tests(void) {
    printf("Starting tests\n");
    UNITY_BEGIN();
    unity_run_tests_by_tag("[hue_json_light]", false);
    UNITY_END();
    UNITY_BEGIN();
    unity_run_tests_by_tag("[hue_json_grouped_light]", false);
    UNITY_END();
    UNITY_BEGIN();
    unity_run_tests_by_tag("[hue_json_smart_scene]", false);
    UNITY_END();
}