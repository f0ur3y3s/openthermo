idf_component_register(
    SRCS "settings.c"
         "settings_logic.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES hvac_logic
                  nvs_flash)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
