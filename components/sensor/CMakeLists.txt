idf_component_register(
    SRCS "sensor.c"
         "sensor_logic.c"
         "sensor_sim.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES board
                  i2c_bus
                  relays
                  esp_timer)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
