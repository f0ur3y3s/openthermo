#!/usr/bin/env python3
"""Source checks for the relay safety path. Fails (exit 1) on any violation.

Runs before every firmware build (tools/matter.sh) and every host test run
(tools/pio_check_safety.py). Also runnable on its own:

    python tools/check_safety_sources.py

What it enforces, over the firmware sources (components/,
matter/components/, matter/main/, matter/bootloader_components/):

  1. Only relays.c (and board.h, which defines them) names the relay pins.
  2. GPIO numbers are written only in board.h, so no other file can reach a
     relay pin by number.
  3. Only relays.c calls gpio_set_level(), the one way to drive a pin.
  4. Only relays.c calls relays_guard_step(); only control.c calls
     relays_apply(). So every output passes the guard, and only the control
     task requests outputs.
  5. In board.h, per target: no relay pin is a strapping, USB, UART TX or
     flash pin (CLAUDE.md, boot rules), and no GPIO is assigned twice.
  6. Nothing calls a plain settings setter: every settings change is one
     settings_update(), so the D-pad and Matter cannot lose each other's
     edits.
"""

import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
SOURCE_DIRS = ("components", "matter/components", "matter/main",
               "matter/bootloader_components")
EXTENSIONS = (".c", ".h", ".cpp")
EXCLUDED = ("components/u8g2/",)

BOARD_H = "components/board/include/board.h"
BOARD_C = "components/board/board.c"  # drives the C6 RF switch only
STATUS_LED_C = "components/status_led/status_led.c"  # bench LED only
RELAYS_C = "components/relays/relays.c"
CONTROL_C = "components/control/control.c"
# Bootloader hook: drops the relay pins after any reset; writes 0 only.
BOOT_HOOK = "matter/bootloader_components/relay_boot_safe/relay_boot_safe.c"

# (pattern, files allowed to contain it, why)
RULES = (
    (r"\bBOARD_PIN_RELAY_", {BOARD_H, RELAYS_C, BOOT_HOOK},
     "relay pins may be named only in relays.c (and the boot hook, which "
     "only drops them)"),
    # Register-level pin access, which bypasses gpio_set_level(). Setting an
    # output high: relays.c only. Clearing outputs and enabling drivers:
    # relays.c and the boot hook, so the hook can only ever drive pins low.
    (r"\b(GPIO_OUT_W1TS_REG|GPIO_OUT_REG|GPIO_OUT1?_W1TS_REG)\b", {RELAYS_C},
     "only relays.c may touch the output-set registers"),
    (r"\bGPIO_(OUT_W1TC|ENABLE_W1TS|ENABLE_W1TC|ENABLE)_REG\b",
     {RELAYS_C, BOOT_HOOK},
     "only relays.c and the boot hook may touch the GPIO output registers"),
    (r"\bgpio_ll_\w+\s*\(", {RELAYS_C},
     "only relays.c may use the GPIO low-level driver"),
    (r"\bREG_WRITE\s*\(", {RELAYS_C, BOOT_HOOK},
     "raw register writes only in relays.c and the boot hook"),
    (r"\bGPIO_NUM_\d+\b", {BOARD_H},
     "GPIO numbers may be written only in board.h"),
    (r"\bgpio_set_level\s*\(", {RELAYS_C, BOARD_C, STATUS_LED_C},
     "only relays.c may drive a pin (board.c: RF switch; status_led.c: "
     "bench LED)"),
    (r"\brelays_guard_step\s*\(", {RELAYS_C, "components/relays/relays_guard.c",
                                   "components/relays/include/relays_guard.h"},
     "only relays.c may call the guard"),
    (r"\brelays_apply\s*\(", {CONTROL_C, RELAYS_C,
                              "components/relays/include/relays.h"},
     "only the control task may request outputs"),
    (r"\bsettings_set\s*\(", set(),
     "settings change only through settings_update(), one locked "
     "read-modify-write, so the D-pad and Matter cannot lose each other's "
     "edits"),
)

# Per target: pins that must never carry a relay.
FORBIDDEN_RELAY_PINS = {
    # ESP32-C6 (IDF soc/esp32c6 and the datasheet): strapping pins 4 (MTMS),
    # 5 (MTDI), 8, 9, 15; USB-Serial-JTAG 12, 13; UART0 TX 16; SPI flash
    # 24..30.
    "CONFIG_IDF_TARGET_ESP32C6": {
        4: "strapping", 5: "strapping", 8: "strapping", 9: "strapping",
        15: "strapping", 12: "USB-JTAG D-", 13: "USB-JTAG D+",
        16: "UART0 TX", 24: "SPI flash", 25: "SPI flash", 26: "SPI flash",
        27: "SPI flash", 28: "SPI flash", 29: "SPI flash", 30: "SPI flash",
    },
}


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def firmware_files():
    for top in SOURCE_DIRS:
        base = os.path.join(ROOT, top)
        for dirpath, _, files in os.walk(base):
            for name in files:
                if name.endswith(EXTENSIONS):
                    path = os.path.join(dirpath, name)
                    rel = os.path.relpath(path, ROOT).replace("\\", "/")
                    if not rel.startswith(EXCLUDED):
                        yield rel


def check_rules(problems):
    for rel in firmware_files():
        with open(os.path.join(ROOT, rel), encoding="utf-8") as handle:
            code = strip_comments(handle.read())
        for pattern, allowed, why in RULES:
            if rel not in allowed and re.search(pattern, code):
                problems.append(f"{rel}: {why} (matched {pattern})")


def board_blocks(text):
    """Splits board.h into {target macro: block text}."""
    blocks = {}
    parts = re.split(r"#(?:el)?if\s+(CONFIG_IDF_TARGET_\w+)", text)
    for macro, body in zip(parts[1::2], parts[2::2]):
        blocks[macro] = re.split(r"#el(?:se|if)", body)[0]
    return blocks


def check_board(problems):
    with open(os.path.join(ROOT, BOARD_H), encoding="utf-8") as handle:
        text = strip_comments(handle.read())
    blocks = board_blocks(text)
    for macro, forbidden in FORBIDDEN_RELAY_PINS.items():
        if macro not in blocks:
            problems.append(f"{BOARD_H}: no pin block for {macro}")
            continue
        pins = re.findall(r"#define\s+(BOARD_PIN_\w+)\s+GPIO_NUM_(\d+)",
                          blocks[macro])
        seen = {}
        for name, num in pins:
            num = int(num)
            if num in seen:
                problems.append(f"{BOARD_H} {macro}: GPIO{num} is both "
                                f"{seen[num]} and {name}")
            seen[num] = name
            if name.startswith("BOARD_PIN_RELAY_") and num in forbidden:
                problems.append(f"{BOARD_H} {macro}: {name} is GPIO{num}, a "
                                f"{forbidden[num]} pin")
        relays = [n for n, _ in pins if n.startswith("BOARD_PIN_RELAY_")]
        if len(relays) != 4:
            problems.append(f"{BOARD_H} {macro}: expected 4 relay pins, "
                            f"found {len(relays)}")


def main():
    problems = []
    check_rules(problems)
    check_board(problems)
    if problems:
        print("check_safety_sources: FAILED")
        for problem in problems:
            print("  " + problem)
        return 1
    print("check_safety_sources: relay path OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
