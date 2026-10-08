#!/usr/bin/env bash
# Installs the firmware toolchain for openthermo inside WSL2 (Debian or
# Ubuntu): ESP-IDF v5.5.5 and esp-matter release/v1.6, which recommends that
# IDF. esp-matter does not build on native Windows, so the firmware (matter/)
# is built from here.
#
# Usage, inside WSL:
#   bash /mnt/e/esp/openthermo/tools/setup_matter_wsl.sh
# Re-running is safe: finished steps are skipped.
#
# Afterwards, in each new shell:
#   source ~/esp/esp-idf/export.sh && source ~/esp/esp-matter/export.sh
set -euo pipefail

IDF_VERSION=v5.5.5
MATTER_BRANCH=release/v1.6
# The bridge relies on esp-matter internals (how reports re-enter the write
# path, the On/Off start-up rule): pin the exact commits it was checked
# against (Oct 2026), not just the branch.
MATTER_COMMIT=c6607128fedc83cb65d6324c14d3e4d7a4d6bd0a
CHIP_COMMIT=93abd8e6891bb578ea63254fb29d099936f345c8
ROOT="${OPENTHERMO_ESP_ROOT:-$HOME/esp}"

echo "== apt prerequisites"
sudo apt-get update
sudo apt-get install -y git wget flex bison gperf python3 python3-pip \
    python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util \
    libusb-1.0-0 pkg-config libglib2.0-dev libgirepository1.0-dev \
    libcairo2-dev libreadline-dev libdbus-1-dev unzip libevent-dev \
    libavahi-client-dev python3-dev clang-tidy

mkdir -p "$ROOT"
cd "$ROOT"

echo "== ESP-IDF $IDF_VERSION"
if [ ! -d esp-idf ]; then
    git clone -b "$IDF_VERSION" --depth 1 --recursive --shallow-submodules \
        https://github.com/espressif/esp-idf.git
fi
./esp-idf/install.sh esp32c6
# shellcheck disable=SC1091
source ./esp-idf/export.sh

echo "== esp-matter $MATTER_BRANCH"
if [ ! -d esp-matter ]; then
    git clone -b "$MATTER_BRANCH" --depth 1 \
        https://github.com/espressif/esp-matter.git
fi
cd esp-matter
if [ "$(git rev-parse HEAD)" != "$MATTER_COMMIT" ]; then
    git fetch --depth 1 origin "$MATTER_COMMIT"
    git checkout --quiet FETCH_HEAD
fi
git submodule update --init --depth 1
if [ "$(git -C connectedhomeip/connectedhomeip rev-parse HEAD)" != "$CHIP_COMMIT" ]; then
    echo "connectedhomeip is not at $CHIP_COMMIT: check before building" >&2
    exit 1
fi
(
    cd connectedhomeip/connectedhomeip
    ./scripts/checkout_submodules.py --platform esp32 linux --shallow
)
./install.sh

echo "== done"
echo "In each new shell: source $ROOT/esp-idf/export.sh && source $ROOT/esp-matter/export.sh"
