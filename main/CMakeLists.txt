if(DEFINED CACHE{HUETEST})
    idf_component_register(SRCS "test.c"
                        INCLUDE_DIRS "include")
else()
    idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS "include")
endif()