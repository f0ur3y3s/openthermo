# Builds Project Nayuki's QR Code generator (C version) as an ESP-IDF
# component, for the commissioning QR code on the OLED.
#
# The sources come from a checkout of nayuki/QR-Code-generator that
# tools/matter.sh clones once, at the version pinned there, and names in
# OPENTHERMO_QRCODEGEN_DIR (an environment variable, so ESP-IDF's early
# requirements pass sees it too). Nothing is vendored into the repo.

set(QRCODEGEN_C_DIR "$ENV{OPENTHERMO_QRCODEGEN_DIR}/c")
if(NOT EXISTS "${QRCODEGEN_C_DIR}/qrcodegen.h")
    message(FATAL_ERROR
            "qrcodegen sources not found at '${QRCODEGEN_C_DIR}'. Build with "
            "tools/matter.sh, which fetches them and sets "
            "OPENTHERMO_QRCODEGEN_DIR.")
endif()

idf_component_register(SRCS "${QRCODEGEN_C_DIR}/qrcodegen.c"
                       INCLUDE_DIRS "${QRCODEGEN_C_DIR}")
