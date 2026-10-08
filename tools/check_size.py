#!/usr/bin/env python3
"""Fails a build that is too close to running out of flash or static RAM.

Run inside the ESP-IDF environment, on a finished idf.py build directory;
tools/matter.sh runs it after every build:

    python tools/check_size.py <build_dir> [--flash-budget 0.90] [--ram-budget 0.80]

Checks:
  flash  the app image against the SMALLEST app partition in the partition
         table the build used (OTA needs every slot to hold the image). The
         budget leaves headroom for growth.
  RAM    static RAM (.data + .bss + IRAM code in the shared DRAM/IRAM
         region) from the linker map, via esp_idf_size. What is left is the
         heap that Wi-Fi/Thread, BLE and Matter allocate from at run time,
         which this cannot see: the firmware logs its real heap low-water
         mark every 10 min (control.c) for that.

Exit 1 if either budget is exceeded.
"""

import argparse
import csv
import glob
import json
import os
import subprocess
import sys


def sdkconfig_value(path, key):
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            if line.startswith(key + "="):
                return line.split("=", 1)[1].strip().strip('"')
    return None


def smallest_app_partition(csv_path):
    smallest = None
    with open(csv_path, encoding="utf-8") as handle:
        for row in csv.reader(handle):
            cells = [cell.strip() for cell in row]
            if not cells or cells[0].startswith("#") or len(cells) < 5:
                continue
            if cells[1] == "app":
                size = int(cells[4], 0)
                smallest = size if smallest is None else min(smallest, size)
    return smallest


def static_ram(map_path):
    """(used, total) bytes of the main data RAM region, from the map."""
    out = subprocess.run(
        [sys.executable, "-m", "esp_idf_size", "--format", "json", map_path],
        check=True, capture_output=True, text=True).stdout
    data = json.loads(out)
    # Chips with a shared DRAM/IRAM region report "diram"; others "dram".
    for region in ("diram", "dram"):
        total = data.get(f"{region}_total", 0)
        if total:
            return data[f"used_{region}"], total, region
    return 0, 0, "none"


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("build_dir")
    parser.add_argument("--flash-budget", type=float, default=0.90)
    parser.add_argument("--ram-budget", type=float, default=0.80)
    args = parser.parse_args()

    with open(os.path.join(args.build_dir, "project_description.json"),
              encoding="utf-8") as handle:
        desc = json.load(handle)
    app_bin = os.path.join(args.build_dir, desc["app_bin"])
    map_path = glob.glob(os.path.join(args.build_dir, "*.map"))[0]
    table = sdkconfig_value(desc["config_file"],
                            "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME")
    csv_path = os.path.join(desc["project_path"], table)

    app_size = os.path.getsize(app_bin)
    slot = smallest_app_partition(csv_path)
    ram_used, ram_total, region = static_ram(map_path)

    flash_ratio = app_size / slot
    ram_ratio = ram_used / ram_total if ram_total else 0.0
    flash_ok = flash_ratio <= args.flash_budget
    # No RAM figures means the size tool's output changed: fail, rather than
    # pass a check that measured nothing.
    ram_ok = (ram_total > 0) and (ram_ratio <= args.ram_budget)

    print(f"check_size: {desc['target']}")
    print(f"  flash  {app_size:>9,} of {slot:,} B app slot "
          f"= {flash_ratio:5.1%} (budget {args.flash_budget:.0%}) "
          f"{'OK' if flash_ok else 'OVER BUDGET'}")
    print(f"  RAM    {ram_used:>9,} of {ram_total:,} B static {region} "
          f"= {ram_ratio:5.1%} (budget {args.ram_budget:.0%}) "
          f"{'OK' if ram_ok else ('NOT MEASURED' if not ram_total else 'OVER BUDGET')}; "
          f"{ram_total - ram_used:,} B left for heap")
    return 0 if (flash_ok and ram_ok) else 1


if __name__ == "__main__":
    sys.exit(main())
