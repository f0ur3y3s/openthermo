# openthermo firmware for the Seeed XIAO ESP32-C6, Matter over Thread: an
# idf.py project, built in WSL2 with esp-matter (tools/setup_matter_wsl.sh
# installs it; tools/matter.sh builds this).
#
# The thermostat itself is the components in ../components (also host-tested
# by PlatformIO); only the Matter bridge (components/matter_bridge) and main/
# live here.
cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{ESP_MATTER_PATH})
    message(FATAL_ERROR "ESP_MATTER_PATH is not set: source ~/esp/esp-matter/export.sh "
                        "(or use tools/matter.sh)")
endif()

# Shown on the info page (esp_app_get_description()->version).
set(PROJECT_VER "0.1.0-matter")
set(PROJECT_VER_NUMBER 1)

set(ESP_MATTER_PATH $ENV{ESP_MATTER_PATH})
set(MATTER_SDK_PATH ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components"
    "${CMAKE_CURRENT_LIST_DIR}/components"
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components")

project(openthermo_matter)

# Bench build without an SHT40 (tools/matter.sh build sim): the
# simulated room of components/sensor/include/sensor_sim.h replaces the
# sensor, and the C6's user LED shows the outputs (status_led.h). The OLED
# shows "SIM". NEVER flash it to the thermostat on the wall.
if(OPENTHERMO_SIM)
    idf_build_set_property(COMPILE_DEFINITIONS "OPENTHERMO_SIM_SENSOR=1" APPEND)
    idf_build_set_property(COMPILE_DEFINITIONS "OPENTHERMO_STATUS_LED=1" APPEND)
endif()

# As esp-matter's examples: C++17 for connectedhomeip, size-optimised.
idf_build_set_property(CXX_COMPILE_OPTIONS
    "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H;-Wno-overloaded-virtual" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
idf_build_set_property(COMPILE_OPTIONS
    "-Wno-format-nonliteral;-Wno-format-security" APPEND)
