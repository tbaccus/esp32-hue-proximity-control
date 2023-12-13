idf_build_get_property(projdir PROJECT_DIR)

if(EXISTS "${projdir}/main/test.c")
    idf_component_register(SRCS "test.c"
                        INCLUDE_DIRS "include"
                        REQUIRES "hue_test_app")
else()
    idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS "include"
                        REQUIRES "hue_helpers")
endif()