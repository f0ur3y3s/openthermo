# Firmware review 2, Oct 2026: critic (safety and compatibility)

Scope: the changes made after `fw_2026-10_synthesis.md`, plus a fresh safety and compatibility pass.

Matter and IDF behaviour was checked in the installed sources (WSL Debian):
- esp-matter `release/v1.6` at c660712;
- connectedhomeip 93abd8e6;
- ESP-IDF v5.5.5.

`python tools/check_safety_sources.py` passes. No builds or `pio test` runs were done.

## Summary

The pure safety core holds up:
- The all-off exemption is narrow, and no non-fault path requests all-off while O = 1.
- The min-run hold-over is correct.
- The settings locking has no deadlock.

The serious problems are at the edges:
1. **E-heat is cancelled on every reboot.** esp-matter's default `StartUpOnOff = Off` on the e-heat plug-in unit writes OnOff = false at every boot. That write goes through the app callback, so the thermostat drops from emergency heat to Heat and runs the compressor 5 min later (C1).
2. **The relays stay energised through a software reset.** On the C6, a panic, task-watchdog or `esp_restart` reset is a CPU-only reset that leaves the GPIO output latches alone. The relays stay driven until `relays_init()`, and indefinitely if startup crash-loops before `app_main` (C2).
3. **The sim build can run the house.** Only documentation stops it being flashed to the installed unit (C3).
4. **Five confirmed fixes from the last synthesis are not in the code** (C4).

## Findings (most severe first)

### C1. E-heat is silently cancelled to Heat on every reboot
- **Severity:** safety.
- **Confidence:** confirmed by tracing the source; not run on hardware.
- **Where:**
  - `matter/components/matter_bridge/matter_bridge.cpp:429-437` (`bridge_create_eheat` sets only `on_off.on_off`);
  - `:175-186` (`bridge_edit`), `:87` (`g_restore_mode` defaults to Heat);
  - `bridge_map.c:212-221`.

**Defect.** The chain, step by step:
1. `on_off_plug_in_unit` adds the OnOff **Lighting** feature (`on_off_plug_in_unit_device.cpp`: `feature::lighting::add`).
2. esp-matter's lighting config defaults `start_up_on_off(0)`, which is **Off** and not null (`generated/clusters/on_off/on_off.h:31`). The attribute is non-volatile and writable.
3. At `esp_matter::start()`, `endpoint::enable_all()` calls `emberAfOnOffClusterServerInitCallback`, which calls `initOnOffServer`.
4. `initOnOffServer` calls `getOnOffValueForStartUp`, which returns false, and then `setOnOffValue(Off)` (`on-off-server.cpp:280-311, 330-360`).
5. `setOnOffValue(Off)` calls `Attributes::OnOff::Set`, which calls `emberAfWriteAttribute` (`esp_matter_ember_stubs.cpp:730`).
6. That calls `set_val_internal(attr, &val)` with the default `call_callbacks = true` (`esp_matter_data_model_priv.h:86`), so POST_UPDATE runs `bridge_attribute_cb`.
7. `g_h_sync` is still nullptr at this point, so the echo skip does not apply. `bridge_edit` maps OnOff = false with mode EHEAT to `g_restore_mode`, which is still its default, Heat.

**Failure scenario.**
1. The outdoor unit is iced or broken, so the user selects emergency heat, from Home or from the D-pad.
2. The sync then reports OnOff = true, and it is persisted because OnOff is NONVOLATILE.
3. A winter power blip, brown-out, watchdog reset or `-app.bin` update happens.
4. On boot, the mode becomes Heat and is saved to NVS 3 s later.
5. Five minutes after boot, the compressor the user had declared unusable runs. W only stages on top of Y1, so if the compressor is dead there is no heat at all.

This defeats the hard rule "emergency heat = Y1 always off", in its intent if not in its letter.

**Fix.**
- Set `cfg.on_off_lighting.start_up_on_off = nullable<uint8_t>()` (null, so the previous value is kept).
- In `bridge_attribute_cb`, ignore POST_UPDATE until Matter has started and the first sync pass has completed. The settings are the truth at boot and Matter never is.
- Refuse controller writes of StartUpOnOff in PRE_UPDATE (see C6).
- Add a bench test: e-heat, reboot, still e-heat.

### C2. A panic, task-watchdog or software reset keeps the relays energised until the next `relays_init()`
- **Severity:** safety.
- **Confidence:** confirmed in IDF source; the crash-loop consequence is plausible.
- **Where:**
  - IDF `esp_system/port/soc/esp32c6/system_internal.c:96-148`: `esp_restart_noos()` resets UART, MSPI, systimer, GDMA, ETM and the crypto blocks, then calls `esp_rom_software_reset_cpu(0)`. There is **no GPIO or IO_MUX reset**; the ETM comment says GPIO state survives a CPU reset.
  - `panic_handler.c:313-321`: C6 panics use `esp_restart_noos()`.
  - Brown-out is different: it uses `esp_rom_software_reset_system()` (`brownout.c:80`), so the pins are reset.
  - Project side: `CLAUDE.md:51`, "Resets: drop the pins"; `relays.c:42-46`; `app.c:84`.

**Defect.** After a task-watchdog panic, `abort()`, `ESP_ERROR_CHECK` or `esp_restart()`, GPIO_OUT and GPIO_ENABLE keep their values:
- through the panic print;
- through the ROM and the 2nd-stage bootloader (which has no GPIO code);
- through image load and C/C++ runtime start-up;

until `relays_init()` writes them low. The 10 k pull-downs do not help, because the pins are actively driven.

**Failure scenarios.**
- **(a) Every watchdog or panic reset.** Y1, W or O stay energised for the length of the panic print plus the boot. That is likely around 1 s, unmeasured. The documented rule is "on a watchdog reset … all outputs go off".
- **(b) A deterministic crash before `app_main`.** Examples: a bad image in the active slot, or a fault in a CHIP static constructor or in heap init. This is a CPU-reset loop in which `relays_init()` never runs. The app disables the bootloader's RTC watchdog early in start-up, so nothing ever does a system reset, and the last outputs stay on indefinitely. For example, W and G stay on with the strips running, regardless of temperature.

**Fix.**
1. Add a bootloader hook (`bootloader_components/…/hooks.c`, `bootloader_before_init()`) that sets the four relay pins as low outputs, or input with pull-down. Every boot path then drops them within milliseconds.
2. Register `esp_register_shutdown_handler()` to drive them low on `esp_restart()`.
3. Correct CLAUDE.md:51 and the README, and add "measure output drop time after an induced TWDT" to the bench checklist.

### C3. The sim build drives the real relay pins from a made-up room; only documentation prevents it reaching the wall
- **Severity:** safety (process).
- **Confidence:** plausible.
- **Where:**
  - `components/relays/relays.c` has no `OPENTHERMO_SIM_SENSOR` gate;
  - `sensor.c:37-60`;
  - `sensor_sim.h:11-18`;
  - `matter/out/` holds `…-esp32c6-app.bin` and `…-esp32c6-sim-app.bin` side by side;
  - README:52 recommends `-app.bin` updates, which keep pairing.

**Defect and scenario.**
1. One wrong file in an esptool command puts the sim image on the installed unit.
2. Pairing and settings survive, so Home looks normal.
3. The control loop then drives the real compressor and strips from a simulated room whose "outdoor" swings 45–100 °F every 2 h. In Heat, the house can get no heat while the sim room is warm; in Auto or Cool, it cools on a fictional schedule.
4. The OLED "SIM" label is the only cue. The panel turns itself off after 10 min.

**Fix.**
- In sim builds, compile the relay pin writes out, or require a bench strap pin read at boot. The LED shows the state anyway.
- Give the sim build a distinct Matter product name or label, so Home shows it.

### C4. Confirmed fixes from the previous synthesis are missing
- **Severity:** safety.
- **Confidence:** confirmed by reading the code.

| Synthesis item | State in the code now | Where |
|---|---|---|
| **A3** plausibility window | Absent. A CRC-valid −49 °F (failed sensor) is acted on: Heat runs Y1 + W continuously, never satisfied. | `sensor_logic.c:96-135` |
| **A4** watchdog subscription must be fatal | Still only logged, so the control loop can run unwatched. | `control.c:176-180` |
| **C-3** crash-loop breaker | Absent. Worse given C2. | `app.c` |
| **A5** forbidden relay pins | Only 8, 9, 15 and 16 are listed. 4, 5, 12, 13 and 24–30 are missing, so re-pinning Y1 to GPIO4 passes the lint. | `check_safety_sources.py:63-66` |
| **D3** size check fails open | `static_ram()` still returns `"none"`, so RAM passes at 0 %. | `check_size.py:63, 87` |

**Fix.** Implement them as specified in the synthesis.

### C5. Loss of heat is silent
- **Severity:** safety (system).
- **Confidence:** plausible.
- **Where:**
  - `hvac_logic.c:551-554`;
  - `matter_bridge.cpp:499-516`;
  - `app.c:24-35`, where `nvs_flash_erase` wipes the whole partition;
  - `settings_logic.c:58`.

**Defect.** Three paths end the same way: the house gets no heat and nothing tells the owner.
- **A sensor fault in winter.** It turns everything off indefinitely, as the hard rule requires. The only sign in Apple Home is a null LocalTemperature, which raises no notification. CONTROL_SPEC asks to "report the fault to Matter", and only the null does that.
- **An NVS corruption or erase.** It wipes the settings, Thread credentials and fabric. The unit comes back in mode Off, unpaired.
- **A downgrade to v1 firmware.** It cannot read the 18-byte v2 record (`ESP_ERR_NVS_INVALID_LENGTH`) and starts in Off.

None of these is listed as a residual in CLAUDE.md. The worst case is frozen pipes in an unattended house.

**Fix.**
- Expose the fault so Home can alert. For example:
  - a Boolean State or contact-sensor "fault" endpoint, which Apple can notify on;
  - or Thermostat events with `kEvents`.
- Document the freeze risk and the downgrade behaviour.
- Consider a "last mode" copy outside the NVS namespace that `nvs_flash_erase` wipes.

### C6. E-heat is an Apple Home "outlet" with Groups, Scenes, Lighting commands and a writable StartUpOnOff
- **Severity:** safety (low) and compatibility.
- **Confidence:** confirmed for the clusters; plausible for the Apple behaviour.
- **Where:** `matter_bridge.cpp:429-437`; the esp-matter `on_off_plug_in_unit` device adds Groups, Scenes Management, Lighting (OnWithTimedOff, StartUpOnOff).

**Defect.**
- "Turn on all the outlets", a room-wide Siri command, or a user scene built from "all accessories in the room" can switch emergency heat on, running 10–20 kW of strips.
- Any admin controller can write StartUpOnOff to On or Toggle, which gives e-heat on every boot or e-heat flipping on every boot.
- PRE_UPDATE refuses only SystemMode (`:226-237`).

**Fix.**
- Refuse StartUpOnOff, OnTime and OffWaitTime writes in PRE_UPDATE.
- Null StartUpOnOff (C1).
- Document how the endpoint should be named and kept out of scenes. Alternatively, find a device type Apple does not group with outlets.

### C7. The `g_restore_mode` writes race
- **Severity:** correctness.
- **Confidence:** plausible, needing a narrow interleaving.
- **Where:** `matter_bridge.cpp:479-485` (sync, outside the settings lock) and `:179-182` (`bridge_edit`, under the lock).

**Scenario.**
1. Sync reads `cfg.mode = Cool`.
2. The CHIP task (higher priority) applies Home "Off", then "e-heat on", storing restore = Off.
3. Sync then stores the stale Cool.
4. E-heat off later restores **Cool**, not Off.

**Fix.** Update `g_restore_mode` only inside a `settings_update` edit, for example a no-op edit run from the sync, so every write is serialised with the mode.

### C8. A flaky sensor gives repeated sub-3-min compressor runs through the fault waiver
- **Severity:** safety (low).
- **Confidence:** plausible.
- **Where:** `sensor_logic.c:216-229`; `hvac_logic.c:551-566`.

**Scenario.**
1. An intermittent I2C link gives one good read, then more than 120 s of failures.
2. After 5 min off, the compressor starts on the good read.
3. The fault 120 s later stops it legally, because a fault waives minimum run.
4. The cycle repeats: 2 min runs every ~7 min, indefinitely.

**Fix.** Use fault-clear hysteresis: require about 60 s of continuous good reads before the fault clears, or before Y1 may start after one.

### C9. The all-off re-arm decisions use two different clocks
- **Severity:** correctness.
- **Confidence:** plausible, a ms-wide window.
- **Where:** `hvac_logic.c:433-438` (the re-arm only if `elapsed < MIN_OFF` on the logic clock) and `relays_guard.c:89-105` (`!b_off_ok` on the guard clock, read later).

**Scenario.**
1. A fault lands within a few ms of the 5 min boundary.
2. The guard re-arms; the logic does not.
3. On recovery, the logic asks for O + Y1 every second for 5 min, and the guard refuses each time. That gives about 300 WARN lines, a refused start each cycle, and a UI showing wait 0.

This is not unsafe, because the guard is the stricter layer.

**Fix.** In `hvac_fault`, re-arm whenever it drops O, with no elapsed test. The logic is then always at least as strict as the guard.

### C10. HVAC compatibility: the O convention is hard-coded and O is held indefinitely in Off
- **Severity:** compatibility and docs.
- **Confidence:** confirmed.
- **Where:** `hvac_logic.h:13-17`; CONTROL_SPEC table; `HARDWARE.md:13,16`.

**Defects.**
- **B-type systems.** The convention is O-energised-in-cool, with no option and no "B systems are unsupported" warning in the README. On a Rheem or Ruud B system:
  - Heat would cool;
  - the call would never be satisfied;
  - aux would stage W on top of a cooling compressor (W + Y1 + O = 0 passes the guard).
- **O held in Off.** "Hold O" also applies to mode Off. After a cooling season, O stays energised all winter in Off. That is a continuous coil load, harmless but unusual; most thermostats drop O/B in Off.
- **Other assumptions to document:** single-stage only (no Y2, W1/W2 jumpered).

**Fix.**
- State these as hard assumptions in the README.
- Optionally add a compile-time `OPENTHERMO_REVERSING_B`.
- Optionally drop O on a transition to Off once the min-off has elapsed.

### C11. Fan Control is code-driven in v1.6, so PercentCurrent reports are invisible and wrong
- **Severity:** compatibility.
- **Confidence:** plausible.
- **Where:** `matter_bridge.cpp:580-582, 496`; esp-matter `data_model_provider/clusters/fan_control/integration.cpp:100`; `FanControlCluster.cpp:115-128`.

**Defect.**
- The FanControl cluster is a registered code-driven server with its own `mPercentCurrent`.
- PercentCurrent is not writable, so `report()` lands in the esp-matter store, which reads do not consult.
- The value reported is the setting, not the actual G.

**Fix.** Use the cluster's `SetPercentCurrent()` from `applied.b_g`, or drop the report.

### C12. Writable limits, deadband and StartUpOnOff are taken from any controller and never re-asserted
- **Severity:** compatibility.
- **Confidence:** plausible.
- **Where:** `matter_bridge.cpp:226-237` (PRE_UPDATE checks SystemMode only); `bridge_sync` does not report limits after boot.

**Scenario.** A controller writes MinSetpointDeadBand = 127. From then on, every setpoint write from Home fails `CheckCoolingSetpointDeadband` (`ThermostatCluster.cpp:381-393`) until a reboot.

**Fix.** Refuse writes to the Min/Max limits, the deadband and StartUpOnOff in PRE_UPDATE.

### C13. esp-matter is tracked by branch, not by commit
- **Severity:** compatibility.
- **Confidence:** confirmed.
- **Where:** `tools/setup_matter_wsl.sh:15-16, 41`.

**Defect.** The bridge's correctness depends on three internal behaviours of esp-matter:
- writable-attribute reports re-entering the callbacks;
- the codegen OnOff StartUp path (C1);
- the code-driven FanControl.

A fresh setup takes whatever `release/v1.6` is that day.

**Fix.** Pin the esp-matter and connectedhomeip commits in the setup script.

### C14. `check_relay_log.py` can false-FAIL near the 5 min boundary
- **Severity:** docs/tooling.
- **Confidence:** plausible.
- **Where:** `tools/check_relay_log.py:79-85`.

**Defect.**
- Log timestamps come from the tick, are taken after `relays_apply`, and can be delayed by preemption.
- The firmware times on `esp_timer` at the start of each cycle.
- A start at exactly 300 000 ms on the firmware clock can therefore log as 299 98x.

**Fix.** Allow about 50 ms of tolerance, or log the guard's own `now_ms` in the line.

## Checked and found sound
- **Guard all-off exemption:** no non-fault path requests all-off while O = 1, because Off and idle hold O. `control.c` never passes NULL. Each exempt drop re-arms min-off (`relays_guard.c:89-105`).
- **Min-run hold-over** (`hvac_logic.c:248-256, 555-562`):
  - waived only for Off, e-heat and a fault;
  - W is dropped in a held heat run under Cool (`:366-374`);
  - Y1 never runs against the valve;
  - a stuck-high reading or over-temperature ends the call after at most 3 min.
- **`hvac_reconcile` with the re-arm:** a refused start restores the previous edge; an unexpected stop re-arms.
- **Guard inverted shadow:** a mismatch drops all outputs and restarts min-off; every pin is rewritten each cycle.
- **Settings mutex:**
  - the edits are pure;
  - it is a FreeRTOS mutex with priority inheritance;
  - there is no nested lock (CHIP lock, then settings lock, never the reverse);
  - the control task's `settings_get` is bounded, well inside the 5 s watchdog.
- **v1 → v2 migration:** correct, sanitised before control starts, saved at the first commit. `nvs_get_blob` accepts a 16-byte record into the 18-byte buffer.
- **Save retry:** does not clobber a newer change.
- **Sync-task echo skip:** verified that writable-attribute `report()` goes through `provider::WriteAttribute`, whose PRE and POST_UPDATE run on the caller's task. Controller writes always arrive on the CHIP task. EnsureDeadband side-writes reach POST_UPDATE *before* the user's own write, so the user's value wins.
- **Assertions and resets:**
  - silent assertions and `ESP_ERROR_CHECK` still `abort()` (`esp_err.h:107-112`);
  - the task watchdog panics at 5 s;
  - brown-out does a full system reset.
- **Pins:** relays on GPIO 1, 2, 21 and 18 avoid the strapping, UART and USB pins. `relays_init()` writes low before `gpio_config`. `check_safety_sources.py` passes.
- **Matter enums:**
  - FanModeSequence 4 = OffHighAuto;
  - CSO 4 accepts SystemMode 0, 1, 3, 4 and 5;
  - LocalTemperature is nullable and null on a fault;
  - the running-state bits are correct.
- **UI:** the panel is forced on (dimmed) during a fault, and the first key only wakes it. `app_ui` has no safety coupling.
- **Loss of Thread or Matter:** local control continues, because Matter starts after the control loop.
