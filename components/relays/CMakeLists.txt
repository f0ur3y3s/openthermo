idf_component_register(
    SRCS "relays.c"
         "relays_guard.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES board
                  esp_driver_gpio
                  esp_timer)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
