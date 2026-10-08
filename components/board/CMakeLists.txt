# The wiring facts for each board (include/board.h), plus board_init() for
# set-up that belongs to no peripheral (the XIAO C6's RF switch).
idf_component_register(
    SRCS "board.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_gpio)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/strict_warnings.cmake)
target_compile_options(${COMPONENT_LIB} PRIVATE ${OPENTHERMO_STRICT_WARNINGS})
