# BENCH ONLY: the XIAO C6's user LED as a relay indicator. Both sources are
# empty unless OPENTHERMO_STATUS_LED is 1 (the C6 bench build only); see
# include/status_led.h.
idf_component_register(
    SRCS "status_led.c"
         "status_led_pattern.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES board
                  esp_driver_gpio)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
