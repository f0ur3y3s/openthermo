# The application: start-up order (app.c) and the UI loop (app_ui.c).
# matter/main/main.c only hands over to app_start().
idf_component_register(
    SRCS "app.c"
         "app_ui.c"
         "boot_guard.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES board
                  buttons
                  control
                  display
                  i2c_bus
                  pages
                  relays
                  sensor
                  settings
                  status_led
                  esp_timer
                  nvs_flash)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
