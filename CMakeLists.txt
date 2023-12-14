cmake_minimum_required(VERSION 3.16)

set(TEST_COMPONENTS hue_json_builder CACHE STRING "List of components to test")
set(COMPONENTS main $CACHE{TEST_COMPONENTS})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hue_proximity_control)