#!/usr/bin/env python3
"""Make the Matter build's compile database usable by clang-tidy.

idf.py writes compile_commands.json into the build directory with the GCC
RISC-V command lines it built the XIAO C6 firmware with. clang understands
most of them, but not a handful of GCC-only flags, and it needs telling which
target and sysroot to parse for. This script rewrites the database for clang
and keeps only this project's own C sources (not ESP-IDF, esp-matter or u8g2).

Runs in WSL2, where the build is. Normally through tools/matter.sh:
    bash tools/matter.sh tidy [sim]
or by hand:
    python3 tools/tidy_db.py <build dir>/compile_commands.json [--run]
"""

import json
import os
import shlex
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT_DIRS = ("components", "matter/components", "matter/main")
EXCLUDED_DIRS = ("components/u8g2",)
OUT_DIR = os.path.join(ROOT, ".tidy")

# GCC options clang rejects or misreads. Anything starting with one of these
# is dropped.
GCC_ONLY_PREFIXES = (
    "-fstrict-volatile-bitfields",
    "-fno-tree-switch-conversion",
    "-fno-shrink-wrap",
    "-fzero-init-padding",
    "-specs",
    "-nostartfiles",
    "-fdiagnostics-color",
    "-fmacro-prefix-map",
    "-ffile-prefix-map",
    "-Werror",
    # esp-matter adds this to every component that links it. Only
    # connectedhomeip's C++ uses it, and as a command-line macro its <header>
    # value trips bugprone-macro-parentheses with no file to blame (D1).
    "-DCHIP_ADDRESS_RESOLVE_IMPL_INCLUDE_HEADER=",
)

# clang only parses here, never generates code. It has a RISC-V backend, so
# it parses for the same target as GCC; -march and -mabi are kept.
CLANG_TARGET_ARGS = [
    "--target=riscv32-unknown-elf",
    "-Wno-unknown-warning-option",
    "-Wno-unused-command-line-argument",
]


def is_project_source(path):
    rel = os.path.relpath(os.path.realpath(path), ROOT).replace("\\", "/")
    in_project = any(rel.startswith(top + "/") for top in PROJECT_DIRS)
    excluded = any(rel.startswith(ex + "/") for ex in EXCLUDED_DIRS)
    return in_project and not excluded and rel.endswith(".c")


def command_args(entry):
    if "arguments" in entry:
        return list(entry["arguments"])
    return shlex.split(entry["command"])


def sysroot_for(compiler):
    # <toolchain>/bin/riscv32-esp-elf-gcc -> <toolchain>/riscv32-esp-elf
    path = compiler if os.path.isabs(compiler) else shutil.which(compiler)
    if path is None:
        sys.exit(f"compiler {compiler} not found: source the ESP-IDF export.sh")
    toolchain = os.path.dirname(os.path.dirname(path))
    return os.path.join(toolchain, "riscv32-esp-elf")


def convert(src_db):
    if not os.path.isfile(src_db):
        sys.exit(f"{src_db} not found: build first (bash tools/matter.sh build)")

    with open(src_db, encoding="utf-8") as handle:
        entries = json.load(handle)

    converted = []
    for entry in entries:
        path = os.path.join(entry["directory"], entry["file"])
        if not is_project_source(path):
            continue
        args = command_args(entry)
        sysroot = sysroot_for(args[0])
        clang_args = ["clang"] + CLANG_TARGET_ARGS
        clang_args.append("--sysroot=" + sysroot)
        # clang's bare-metal driver does not add the sysroot's include
        # directory by itself, so newlib's headers need naming.
        clang_args.append("-isystem" + os.path.join(sysroot, "include"))
        clang_args += [a for a in args[1:] if not a.startswith(GCC_ONLY_PREFIXES)]
        converted.append(
            {"directory": entry["directory"], "file": path, "arguments": clang_args}
        )

    os.makedirs(OUT_DIR, exist_ok=True)
    out_db = os.path.join(OUT_DIR, "compile_commands.json")
    with open(out_db, "w", encoding="utf-8") as handle:
        json.dump(converted, handle, indent=1)

    print(f"{len(converted)} project sources -> {out_db}")
    return [c["file"] for c in converted]


def main():
    args = [a for a in sys.argv[1:] if a != "--run"]
    if len(args) != 1:
        sys.exit(__doc__)
    files = convert(args[0])
    if "--run" in sys.argv[1:]:
        tidy = os.environ.get("CLANG_TIDY", "clang-tidy")
        result = subprocess.run([tidy, "-p", OUT_DIR, "--quiet"] + files, check=False)
        sys.exit(result.returncode)


if __name__ == "__main__":
    main()
