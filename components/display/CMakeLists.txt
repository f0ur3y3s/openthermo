idf_component_register(
    SRCS "display.c"
         "display_hal.c"
         "display_shift.c"
    INCLUDE_DIRS "include"
    REQUIRES u8g2
    PRIV_REQUIRES board
                  i2c_bus
                  esp_timer)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
