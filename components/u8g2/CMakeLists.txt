# Builds the framework-agnostic C core of U8g2 as an ESP-IDF component.
#
# The sources come from a checkout of olikraus/U8g2_Arduino that
# tools/matter.sh clones once, at the version pinned there, and names in
# OPENTHERMO_U8G2_DIR. That repository's own CMakeLists.txt REQUIRES "arduino"
# plus the U8g2lib/U8x8lib C++ wrappers that #include <Arduino.h>, so we
# ignore it and compile src/clib ourselves. Nothing is vendored into the repo.
#
# An environment variable, not a CMake one, so that ESP-IDF's early
# requirements pass (a separate `cmake -P` run) sees it too.

set(U8G2_CLIB_DIR "$ENV{OPENTHERMO_U8G2_DIR}/src/clib")
if(NOT EXISTS "${U8G2_CLIB_DIR}/u8g2.h")
    message(FATAL_ERROR
            "U8g2 sources not found at '${U8G2_CLIB_DIR}'. Build with "
            "tools/matter.sh, which fetches them and sets OPENTHERMO_U8G2_DIR.")
endif()

file(GLOB U8G2_SRCS "${U8G2_CLIB_DIR}/*.c")
# mui*.c is U8g2's menu-system add-on: unused here and slow to compile.
list(FILTER U8G2_SRCS EXCLUDE REGEX "/mui(_u8g2)?\.c$")

idf_component_register(SRCS ${U8G2_SRCS}
                       INCLUDE_DIRS "${U8G2_CLIB_DIR}")
