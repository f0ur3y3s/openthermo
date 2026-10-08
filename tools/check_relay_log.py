#!/usr/bin/env python3
"""Checks a serial log from the real firmware against the hard timing rules.

The control task logs one line per change of the driven outputs, e.g.

    I (312004) control: Y1 1 G 1 O 0 W 0 | call heat | 662 tenths F

with the time since boot in ms. This replays those lines and checks, on the
firmware's own timestamps:

  FAIL  Y1 starts less than 5 min after it went off (or after boot, or
        after a fault dropped O early)
  FAIL  O changes while Y1 runs, or less than 5 min after Y1 went off,
        except on a FAULT line with every output off (that restarts the
        5 min lockout, as the firmware does)
  FAIL  any output on during a FAULT
  FAIL  W with Y1 + O (strips while cooling)
  FAIL  Y1 or W without G
  FAIL  a run shorter than 3 min that ended with the call still heat or
        cool: the logic holds the call for the minimum run, so only a
        fault, Off or emergency heat may end one sooner
  WARN  a run shorter than 3 min that ended with call none or e-heat (mode
        Off or emergency heat, which the spec allows)

A reboot ("app: reset reason" in the log) drops every output and restarts
the 5 min lockout, exactly as the firmware does. The log's millisecond
timestamps are 32-bit and wrap after 49.7 days; the wrap is undone.

Usage:
    pio device monitor -p COM10 -b 115200 | tee bench.log      (let it run)
    python tools/check_relay_log.py bench.log
Exit status 1 if any FAIL.
"""

import re
import sys

MIN_OFF_MS = 300000
MIN_RUN_MS = 180000
WRAP_MS = 1 << 32  # esp_log_timestamp() is a uint32_t of ms
# The log's timestamps are the logging task's, a few ms off the guard's own
# clock (esp_timer) that actually times the rules: allow that much.
TOLERANCE_MS = 50

OUTPUTS = re.compile(
    r"\((\d+)\) control: Y1 (\d) G (\d) O (\d) W (\d) \| call (\S+)( FAULT)?")
BOOT = re.compile(r"app: reset reason: (.*)$")


def check(lines):
    fails, warns = [], []
    boots, starts, o_changes = 0, 0, 0
    shortest_off, shortest_run = None, None
    prev = {"y1": 0, "g": 0, "o": 0, "w": 0}
    y1_off_ms, y1_on_ms = 0, 0
    last_raw, wraps = 0, 0

    for number, line in enumerate(lines, 1):
        boot = BOOT.search(line)
        if boot:
            boots += 1
            prev = {"y1": 0, "g": 0, "o": 0, "w": 0}
            y1_off_ms = 0  # boot arms the lockout
            y1_on_ms = 0
            last_raw, wraps = 0, 0
            continue
        match = OUTPUTS.search(line)
        if not match:
            continue
        raw = int(match.group(1))
        if raw < last_raw:
            wraps += 1  # the 32-bit timestamp wrapped (no reboot between)
        last_raw = raw
        t = raw + (wraps * WRAP_MS)
        now = dict(zip(("y1", "g", "o", "w"),
                       (int(match.group(i)) for i in range(2, 6))))
        call = match.group(6)
        fault = bool(match.group(7))
        all_off = not any(now.values())
        where = f"line {number} ({t} ms)"

        if now["y1"] and not prev["y1"]:
            starts += 1
            off_for = t - y1_off_ms
            shortest_off = off_for if shortest_off is None else min(shortest_off, off_for)
            if off_for < MIN_OFF_MS - TOLERANCE_MS:
                fails.append(f"{where}: Y1 started {off_for} ms after it went off")
            y1_on_ms = t
        if not now["y1"] and prev["y1"]:
            ran = t - y1_on_ms
            if not fault:
                shortest_run = ran if shortest_run is None else min(shortest_run, ran)
                if ran < MIN_RUN_MS - TOLERANCE_MS and call in ("heat", "cool"):
                    fails.append(f"{where}: Y1 ran only {ran} ms and the call "
                                 f"is still {call}")
                elif ran < MIN_RUN_MS:
                    warns.append(f"{where}: Y1 ran only {ran} ms "
                                 f"(call {call}: Off or e-heat)")
            y1_off_ms = t
        if fault and not all_off:
            fails.append(f"{where}: output on during a FAULT")
        if now["o"] != prev["o"]:
            o_changes += 1
            early = prev["y1"] or (t - y1_off_ms) < MIN_OFF_MS - TOLERANCE_MS
            if early and fault and all_off:
                y1_off_ms = t  # a fault drop restarts the lockout
            elif prev["y1"]:
                fails.append(f"{where}: O changed while Y1 was running")
            elif early:
                fails.append(f"{where}: O changed {t - y1_off_ms} ms after Y1 off")
        if now["w"] and now["y1"] and now["o"]:
            fails.append(f"{where}: W with Y1 + O")
        if (now["y1"] or now["w"]) and not now["g"]:
            fails.append(f"{where}: Y1 or W without G")
        prev = now

    return {
        "boots": boots, "starts": starts, "o_changes": o_changes,
        "shortest_off_ms": shortest_off, "shortest_run_ms": shortest_run,
        "fails": fails, "warns": warns,
    }


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    with open(sys.argv[1], encoding="utf-8", errors="replace") as handle:
        result = check(handle.readlines())

    print(f"boots {result['boots']}, Y1 starts {result['starts']}, "
          f"O changes {result['o_changes']}")
    print(f"shortest Y1 off before a start: {result['shortest_off_ms']} ms "
          f"(rule >= {MIN_OFF_MS})")
    print(f"shortest Y1 run: {result['shortest_run_ms']} ms "
          f"(rule >= {MIN_RUN_MS})")
    for warn in result["warns"]:
        print("WARN " + warn)
    for fail in result["fails"]:
        print("FAIL " + fail)
    if result["starts"] == 0:
        print("note: no compressor starts in this log; nothing was tested")
    print("RESULT: " + ("FAIL" if result["fails"] else "PASS"))
    return 1 if result["fails"] else 0


if __name__ == "__main__":
    sys.exit(main())
