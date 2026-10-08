# Page drawing (pages.c) plus the pure, host-tested UI logic (pages_nav.c)
# and text formatting (page_fmt.c).
idf_component_register(
    SRCS "pages.c"
         "pages_nav.c"
         "page_fmt.c"
    INCLUDE_DIRS "include"
    REQUIRES buttons
             control
             settings
    PRIV_REQUIRES display
                  hvac_logic
                  esp_app_format)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
