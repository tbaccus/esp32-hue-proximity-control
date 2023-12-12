cmake_minimum_required(VERSION 3.16)

if(DEFINED CACHE{HUETEST})
    set(TEST_COMPONENTS "hue_json_builder" CACHE STRING "List of components to test")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hue_proximity_control)