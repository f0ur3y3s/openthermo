# Bootloader hook: drives the relay pins low first thing after any reset
# (relay_boot_safe.c). ESP-IDF builds every component in the project's
# bootloader_components/ into the 2nd-stage bootloader.
idf_component_register(
    SRCS "relay_boot_safe.c"
    PRIV_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../../components/board/include"
    PRIV_REQUIRES esp_rom
                  soc)

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
