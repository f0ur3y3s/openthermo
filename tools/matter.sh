#!/usr/bin/env bash
# Builds the firmware (matter/) for the Seeed XIAO ESP32-C6 inside WSL2. Run
# from anywhere:
#
#   bash tools/matter.sh build [debug|sim]
#                                         build and write the flashable images
#                                         to matter/out/. The plain build is
#                                         the release: no serial shell.
#                                         "debug" adds the shell (`matter esp
#                                         ...` commands: factory reset,
#                                         attributes, the bench reset checks).
#                                         "sim" is a debug build that also
#                                         swaps the SHT40 for the simulated
#                                         room and blinks the user LED (bench
#                                         only, never on the wall)
#   bash tools/matter.sh tidy [debug|sim] clang-tidy over the project's own C
#                                         sources (builds first if needed)
#   bash tools/matter.sh menuconfig [...] idf.py menuconfig for the same build
#   bash tools/matter.sh clean [...]      remove the build directory
#
# The build directory lives in the WSL home (fast), not on the Windows
# drive. Flashing is from Windows; see README.md.
set -euo pipefail

ROOT="${OPENTHERMO_ESP_ROOT:-$HOME/esp}"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../matter" && pwd)"
TOOLS_DIR="$PROJECT_DIR/../tools"
TARGET=esp32c6
CMD="${1:-build}"
shift || true

VARIANT="$TARGET"
SIM_ARGS=()
DEFAULTS="sdkconfig.defaults" # ESP-IDF adds sdkconfig.defaults.esp32c6 too
for arg in "$@"; do
    case "$arg" in
        debug)
            VARIANT="$TARGET-debug"
            DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.debug"
            ;;
        sim)
            VARIANT="$TARGET-sim"
            SIM_ARGS=(-D OPENTHERMO_SIM=1)
            DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.debug"
            ;;
        esp32c6) ;; # the only target; accepted for old command lines
        *)
            echo "unknown argument: $arg" >&2
            exit 2
            ;;
    esac
done
BUILD_DIR="$HOME/build/openthermo-matter-$VARIANT"
OUT_DIR="$PROJECT_DIR/out"

# U8g2's C core, pinned. components/u8g2 builds it from this checkout.
U8G2_VERSION=2.36.18
export OPENTHERMO_U8G2_DIR="$ROOT/u8g2-$U8G2_VERSION"
if [ ! -d "$OPENTHERMO_U8G2_DIR/src/clib" ]; then
    git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$U8G2_VERSION" \
        https://github.com/olikraus/U8g2_Arduino "$OPENTHERMO_U8G2_DIR"
fi

# Project Nayuki's QR Code generator, pinned, for the pairing page's QR code.
QRCODEGEN_VERSION=v1.8.0
export OPENTHERMO_QRCODEGEN_DIR="$ROOT/qrcodegen-$QRCODEGEN_VERSION"
if [ ! -f "$OPENTHERMO_QRCODEGEN_DIR/c/qrcodegen.c" ]; then
    git -c advice.detachedHead=false clone --quiet --depth 1 \
        --branch "$QRCODEGEN_VERSION" \
        https://github.com/nayuki/QR-Code-generator "$OPENTHERMO_QRCODEGEN_DIR"
fi

# The export scripts read variables that may be unset, so relax -u for them.
set +u
# shellcheck disable=SC1091
source "$ROOT/esp-idf/export.sh" > /dev/null 2>&1
# shellcheck disable=SC1091
source "$ROOT/esp-matter/export.sh" > /dev/null 2>&1
set -u

# Each variant keeps its sdkconfig with its build, so sim and real builds do
# not overwrite each other's configuration.
IDF=(idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" -D "SDKCONFIG=$BUILD_DIR/sdkconfig"
     -D "SDKCONFIG_DEFAULTS=$DEFAULTS" -D "IDF_TARGET=$TARGET" "${SIM_ARGS[@]}")

case "$CMD" in
    build)
        python3 "$TOOLS_DIR/check_safety_sources.py"
        "${IDF[@]}" build
        # Matter, Thread and BLE are fixed costs on the 4 MB C6, so the
        # budget is 90%. Revisit when the lean (no shell, fewer logs) build
        # exists.
        python3 "$TOOLS_DIR/check_size.py" "$BUILD_DIR" --flash-budget 0.90
        mkdir -p "$OUT_DIR"
        "${IDF[@]}" merge-bin -o "$OUT_DIR/openthermo-matter-$VARIANT.bin"
        cp "$BUILD_DIR/openthermo_matter.bin" "$OUT_DIR/openthermo-matter-$VARIANT-app.bin"
        cp "$BUILD_DIR/bootloader/bootloader.bin" "$OUT_DIR/openthermo-matter-$VARIANT-bootloader.bin"
        echo "First flash, or start clean, at 0x0: matter/out/openthermo-matter-$VARIANT.bin"
        echo "  (fills the settings area: forgets pairing and settings)"
        echo "Update, keeping all of that, at 0x20000: matter/out/openthermo-matter-$VARIANT-app.bin"
        echo "  plus, when the bootloader changed, at 0x0: matter/out/openthermo-matter-$VARIANT-bootloader.bin"
        ;;
    tidy)
        if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
            "${IDF[@]}" reconfigure
        fi
        # LLVM's apt repository installs it versioned; Debian's is plain.
        CLANG_TIDY="${CLANG_TIDY:-$(command -v clang-tidy-18 || echo clang-tidy)}" \
            python3 "$TOOLS_DIR/tidy_db.py" "$BUILD_DIR/compile_commands.json" --run
        ;;
    menuconfig)
        "${IDF[@]}" menuconfig
        ;;
    clean)
        rm -rf "$BUILD_DIR"
        ;;
    *)
        echo "usage: $0 {build|tidy|menuconfig|clean} [debug|sim]" >&2
        exit 2
        ;;
esac
