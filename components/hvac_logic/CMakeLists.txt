# Pure logic: no ESP-IDF calls, host-tested in test/test_hvac_logic.
idf_component_register(
    SRCS "hvac_logic.c"
    INCLUDE_DIRS "include")

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
