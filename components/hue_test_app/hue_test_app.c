#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "hue_test_app.h"

void run_tests(void) {
    printf("Starting tests\n");
    UNITY_BEGIN();
    unity_run_tests_by_tag("[hue_json_light]", false);
    UNITY_END();
}