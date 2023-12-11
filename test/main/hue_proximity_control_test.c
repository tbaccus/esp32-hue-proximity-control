#include <stdio.h>
#include <string.h>
#include "unity.h"

void app_main(void) {
    printf("Starting tests\n");
    UNITY_BEGIN();
    unity_run_tests_by_tag("[light]", false);
    UNITY_END();
    unity_run_menu();
}