# Strict compiler warnings for this project's own components.
#
# Each first-party component includes this file and applies the list to its
# own library target only, so third-party code (components/u8g2) and ESP-IDF
# itself keep their usual flags. -Werror turns every warning into a build
# failure: a warning is either fixed or, with a written reason, suppressed at
# the narrowest scope possible.

set(OPENTHERMO_STRICT_WARNINGS
    -Wall
    -Wextra
    -Werror
    -Wshadow
    -Wconversion
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wpointer-arith
    -Wcast-qual)

# ESP-IDF's own headers are third-party code (CODING_STANDARD.md, D1). On the
# RISC-V chips with privilege modes (the C6), esp_cpu.h has an inline function
# whose CSR macro (riscv/csr.h) converts unsigned long to int, which our
# -Wconversion turns into an error in every file that includes FreeRTOS.
# Marking those two IDF header directories as system headers silences
# warnings *inside them* only; our own code keeps every flag above.
if(CONFIG_IDF_TARGET_ARCH_RISCV)
    list(APPEND OPENTHERMO_STRICT_WARNINGS
         "-isystem$ENV{IDF_PATH}/components/esp_hw_support/include"
         "-isystem$ENV{IDF_PATH}/components/riscv/include")
endif()
