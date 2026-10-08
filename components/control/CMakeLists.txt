idf_component_register(
    SRCS "control.c"
    INCLUDE_DIRS "include"
    REQUIRES hvac_logic
             sensor
    PRIV_REQUIRES relays
                  settings
                  esp_timer)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
