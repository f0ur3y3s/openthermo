# The Matter bridge: the C++ glue to esp-matter (matter_bridge.cpp) and the
# pure, host-tested value mapping (bridge_map.c).
#
# The strict warning flags apply to bridge_map.c only. matter_bridge.cpp
# includes esp-matter and connectedhomeip headers that do not build under
# -Wconversion -Werror; see docs/CODING_STANDARD.md, deviation D9.
idf_component_register(
    SRCS "matter_bridge.cpp"
         "bridge_map.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES control
                  hvac_logic
                  settings
                  sensor
                  esp_matter
                  esp_matter_console
                  esp_timer)

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/strict_warnings.cmake)
set_source_files_properties(bridge_map.c PROPERTIES
                            COMPILE_OPTIONS "${OPENTHERMO_STRICT_WARNINGS}")
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
