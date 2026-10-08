# Firmware review, Oct 2026: critic

Scope: components/*, matter/ (bridge, main, sdkconfig, partitions), tools/, test/, docs. Read-only. I ran `python tools/check_safety_sources.py` (passes) and `tools/check_size.py` against the existing C6 build directory (82.4% flash, 40.7% static RAM; nothing was rebuilt). To confirm C3, I read the esp-matter `release/v1.6` sources in WSL (`~/esp/esp-matter`).

## Summary

The relay path holds up well. The guard is independent, the inverted shadow works, the clocks are 64-bit and monotonic, the pins go low first, and resets are fail-safe. I found no path that starts Y1 early or changes O while Y1 runs.

I did find these problems:

- **Two places where the code contradicts the hard rules as written.**
  - On a sensor fault, O is *not* dropped (C1).
  - Minimum run is waived on Heat↔Cool mode changes, which can come from Matter (C2).
- **One Matter defect.** Valid extreme setpoints cannot be reported, and the failed report is never retried (C3).
- **A lost-update race** between the D-pad and Matter on the settings record (C4).
- **Some tooling, test and docs gaps.**

## Findings (most severe first)

### C1. Safety (rule conflict). Confirmed. A sensor fault leaves O energised for up to 5 min

- **Where:**
  - `components/hvac_logic/hvac_logic.c:433-445` (`hvac_fault` zeroes O)
  - `components/relays/relays_guard.c:94-98` (the guard refuses the O change)
  - `components/relays/include/relays_guard.h:8` ("Turning anything off is always allowed")
  - `components/relays/relays.c:78-84`
- **Defect.**
  - `hvac_fault()` asks for every output off, O included, in the same cycle that Y1 stops.
  - The guard treats O 1→0 as an "O change" and refuses it unless Y1 has been off for 5 min. It then holds O at its old value.
  - The two layers disagree:
    - the hard rule says "sensor fault → all outputs off";
    - the README says "Every fault drops all outputs";
    - the guard header claims turning anything off is always allowed.
- **Scenario.**
  1. While cooling (Y1+O+G), the SHT40 stops answering for 2 min.
  2. The fault fires. Y1 and G drop, but O stays energised for 5 more minutes.
  3. Every 1 s cycle logs `guard changed request ... O0 -> O1`, about 300 warnings.
- **Physical risk.** Low, because O is harmless with the compressor off. But the code breaks the documented rule, and the tests miss it:
  - `test_turning_off_is_always_allowed` never turns O off;
  - `test_safety_chain` never checks that a fault drives everything off.
- **Fix.** Decide which rule takes precedence, then make both layers and the docs agree. Either:
  - have `hvac_fault` hold O (as idle does), and correct the README and the guard header; or
  - give the guard an explicit "everything off" request that is allowed to drop O.

### C2. Safety / spec. Confirmed. Minimum run is waived on Heat↔Cool mode changes

- **Where:**
  - `components/hvac_logic/hvac_logic.c:248-254` (`hvac_min_run_holds` requires `hvac_mode_permits`)
  - `hvac_logic.c:287-290` (Y1 is dropped to wait for the valve)
  - `hvac_logic.h:23-25`
  - `README.md:159`
- **Defect.**
  - CONTROL_SPEC allows the 3 min minimum run to be overridden only by "a fault or mode-off". CLAUDE.md lists "Minimum run is 3 min" with no exceptions.
  - `hvac_logic` also waives it on Heat→Cool, Cool→Heat and →e-heat. The e-heat case is defensible ("Y1 always off"); Heat↔Cool is not.
  - The guard does not enforce minimum run at all, so only one layer does. The README's "These rules are enforced twice" (listing the 3 min run) is therefore false.
- **Scenario.**
  1. A Home automation or user flips Heat→Cool→Heat.
  2. After each 5 min lockout the compressor starts, and the next flip kills it within seconds.
  3. The result is runs of about 1 s every 5 min.
- **Tests.** The chain test hides this: `test_safety_chain/test_main.c:169` waives the run check on *every* mode change, including a change to the same mode.
- **Fix.**
  - On a direction change, keep Y1 running in the old direction until the minimum run expires. The O wait already follows.
  - Waive the run only for Off, a fault and e-heat, and document the e-heat case as a hard-rule precedence.
  - Correct the README.

### C3. Correctness (Matter). Confirmed. The thermostat's extreme setpoints cannot be reported, and the failure is never retried

- **Where:**
  - `matter/components/matter_bridge/bridge_map.c:92-107`
  - `matter_bridge.cpp:361-370`, `411-422`, `449`
- **Defect.** `bridge_map_limits()` rounds each limit one hundredth *inward*. The thermostat's own end values therefore convert to just *outside* the Matter limits:

  | Setpoint | Reported (0.01 °C) | Matter limit (0.01 °C) | Result |
  |---|---|---|---|
  | heat 60 °F | 1556 | min 1557 | below |
  | heat 76 °F | 2444 | max 2443 | above |
  | cool 64 °F | 1778 | min 1779 | below |
  | cool 80 °F | 2667 | max 2666 | above |

- **Why the report fails.** esp-matter v1.6 runs with the legacy data model (`CONFIG_ESP_MATTER_ENABLE_GENERATED_DATA_MODEL` is not set).
  - Its `thermostat::add_bounds_cb` bounds `OccupiedHeating/CoolingSetpoint` to `Min/Max*SetpointLimit` when the server starts.
  - `attribute::set_val` returns `ESP_ERR_INVALID_ARG` for any value out of bounds, and `report()` fails with it.
  - `bridge_sync` logs a warning and still records the value in `g_reported`, so it never tries again.
- **Scenario.** On the D-pad, the user lowers heat from 61 °F to 60 °F, or raises cool so the deadband pushes heat to 76 °F. Apple Home keeps showing 61 °F (or the old value) until the setpoint changes again.
- **Tests.** `test_bridge_map` checks only that the limits map back inside the thermostat's ranges. It never checks the reverse.
- **Fix.**
  - Round the limits *outward* and clamp the snapped Matter write with `hvac_setpoints_apply`, which is already done. Or clamp the reported setpoint into `[min_c100, max_c100]`.
  - In any case, update `g_reported` only for reports that succeeded.

### C4. Concurrency. Plausible. A D-pad press and a Matter write can lose each other's update

- **Where:**
  - `components/app/app_ui.c:76-84`
  - `matter/components/matter_bridge/matter_bridge.cpp:159-200`
  - `components/settings/settings.c:380-400`
- **Defect.**
  - Both writers read, modify and write the *whole* `settings_t`: `settings_get`, then a change, then `settings_set`.
  - Nothing holds the lock across the read-modify-write.
  - The CHIP task runs at priority 1, the same as the UI task, so either can be preempted inside that window.
- **Scenario.**
  1. The user presses Up on the main page in Heat.
  2. Between the UI's `settings_get` and `settings_set`, Home writes SystemMode=Off.
  3. The UI's `settings_set` restores mode Heat with the new setpoint, so the Off is silently lost.
  4. `bridge_sync` then reports Heat back to Home, and the system keeps heating.
- **Fix.** Add `settings_update(fn, ctx)`, which applies a mutation under the settings mutex, or use field-wise setters.

### C5. Robustness. Plausible. A boot loop after `control_start` keeps the compressor locked out forever

- **Where:**
  - `components/app/app.c:73-108`
  - `matter_bridge_start`
  - `sdkconfig.defaults:23` (silent assertions still abort)
- **Defect.**
  - Every reset re-arms the 5 min minimum-off, which is correct.
  - But suppose any reproducible abort happens within 5 min of boot. Examples: an OpenThread or connectedhomeip assert from corrupted NVS state, or an `ESP_ERROR_CHECK`.
  - Then the unit reboots for ever with Y1 never allowed. In Heat, the house gets no heat at all, and nothing tells the user why: there is no persistent fault and the OLED only flashes "starting...".
- **Fix.** Add an RTC_NOINIT crash counter. After N resets within M minutes, skip `p_net_start` (run locally) and show "NET DISABLED" on the OLED.

### C6. Correctness / UX. Confirmed. Turning e-heat off always switches to Heat

- **Where:** `matter/components/matter_bridge/bridge_map.c:181-200`
- **Defect.** E-heat on overwrites the stored mode. Off maps `EHEAT→HEAT` whatever the mode was before.
- **Scenario.**
  1. The system is in Cool or Off.
  2. The user turns the "Emergency heat" outlet on by mistake, then off again.
  3. The thermostat is now in Heat and starts heating.
- **Fix.** Remember the mode that was active before e-heat (in RAM, or in `settings_t` with a version bump) and restore it.

### C7. Correctness (Matter). Confirmed. Matter attributes can stay out of step with the thermostat

- **Where:** `matter_bridge.cpp:373-450`
- **Defect.** `bridge_sync` compares against what it *last reported*, not against the attribute's current value. A Matter write that does not change the settings therefore leaves the written value in place:
  - a setpoint that snaps to the current value: 2010 is written, the thermostat stays at 68.0 °F, and Matter keeps 68.2 °F;
  - FanMode Off, Low or Medium while the fan is already Auto or On;
  - an e-heat Off while not in e-heat.
- Combined with C3 (`g_reported` is updated on a failed report), such a mismatch persists until the next real change.
- **Fix.** After a POST_UPDATE that changed nothing, clear `g_reported.b_valid` (or that field) so the next sync re-asserts the thermostat's value.

### C8. Correctness (Matter). Plausible, low. The thermostat's 3 °F deadband is narrower than Matter's 1.7 °C

- **Where:**
  - `bridge_map.h:51` (`BRIDGE_DEADBAND_C10 17`)
  - `hvac_logic.h:71`
- **Defect.** A 3 °F gap is 167 hundredths of a °C, which is less than 170. Every setpoint pair the D-pad pushes to exactly 3 °F apart is reported to Matter in breach of the cluster's own MinSetpointDeadBand invariant.
- **Consequence.** The cluster's `EnsureDeadband` logic then re-pushes the other setpoint on the next Home write, or rejects that write near the limits.
- **Fix.** Make `HVAC_AUTO_DEADBAND_F10` 31, or accept the mismatch and document it.

### C9. Robustness. Confirmed, low. A failed settings save is never retried

- **Where:** `components/settings/settings.c:176-189`
- **Defect.** `g_b_dirty` is cleared before `nvs_set_blob` and `nvs_commit`. On failure (for example NVS full, since Matter, OpenThread and settings share 48 KB) the change is only logged.
- **Consequence.** At the next reboot the mode or setpoint reverts with no sign to the user.
- **Fix.** Set `g_b_dirty` again, with a back-off, on error.

### C10. Tooling. Plausible. `check_safety_sources.py` enforces less than it claims

- **Where:**
  - `tools/check_safety_sources.py:39-59`
  - `docs/HARDWARE.md:65`
- **Missing pins.** The C6 forbidden list (and HARDWARE.md) omits two groups:
  - the strapping pins MTMS and MTDI (GPIO4 and GPIO5; the C6 datasheet lists five strapping pins);
  - the USB D-/D+ pins (GPIO12 and GPIO13), which the USB-Serial-JTAG drives at boot.
- **Bypasses.** Rule 2 ("no other file can reach a relay pin by number") matches only the `GPIO_NUM_<n>` spelling. All of these pass:
  - `(gpio_num_t)18` or `1ULL << 18`;
  - `gpio_config` with `GPIO_MODE_OUTPUT`;
  - `gpio_set_direction`, `gpio_reset_pin`;
  - `gpio_ll_*` calls and `REG_WRITE(GPIO_OUT_W1TS…)`.
- **Fix.**
  - Add 4, 5, 12 and 13.
  - Flag `gpio_config`, `gpio_set_direction`, `gpio_ll_`, `GPIO_OUT_W1T` and raw `1ULL <<` outside the allowed files.
  - Reword the claim.

### C11. Tests. Confirmed. The proofs do not cover what test/README claims

- **What is missing:**
  - `test_safety_chain`:
    - never checks "fault → all outputs off" (it would fail on O; see C1);
    - its faults are single-step blips, not a 2 min loss;
    - it waives the minimum-run check after any mode change, including no-ops;
    - the guard clock is only ever later than the logic's, never earlier (lines 116, 160-169).
  - `test_turning_off_is_always_allowed` does not exercise O.
  - `test_bridge_map` lacks the inverse-limit check (C3).
- **Consequence.** test/README's "every hard rule checked on the pins' clock" overstates the coverage.

### C12. Docs and maintainability. Confirmed

- **CODING_STANDARD still carries text from deskmate:**
  - rule 2 cites `net_bring_up_stack()`;
  - rule 9 mandates `app_events` bits, which do not exist (`display.h:7` repeats "app event");
  - D1 cites `test/test_weather` and cJSON;
  - D3 says "The clock cannot run…";
  - D5 describes `settings_defaults.h` and a recovery AP.
- **CLAUDE.md:**
  - line 45 says the commissioning QR is shown on the OLED, but line 97 says not started;
  - line 44 says "version and a migration", but no migration exists;
  - line 42 says "tenths of °F or °C", but only °F is stored.
- **CONTROL_SPEC.md:47:** lists an "info / commission QR" page, but there is no QR.
- **`matter_bridge.cpp:574-577`:** still registers the Wi-Fi shell commands, and its comment still says "forgets the pairing and Wi-Fi", on a build that is Thread only now that the S3 has been removed.
- **README.md:159-160:** "enforced twice" and "every fault drops all outputs" are wrong (C1, C2).
- **`tools/check_size.py:209-214`:** silently returns 0/0 and passes the RAM budget if the `esp_idf_size` JSON keys change. It should fail instead.
- **`tools/check_relay_log.py`:** uses 32-bit ESP log ms timestamps, which wrap after 49.7 days and would then produce false FAILs on long bench logs.

## Checked and found sound

- `relays_init` writes the output latches low before enabling the drivers. It is the first call in `app_start`, and `board_init` comes after it.
- The relay pins are GPIO1, 2, 21 and 18 on the C6. None is strapping (8, 9 or 15), UART TX (16), USB or the RF switch. The CAD module hole pattern (67 × 44.5) matches HARDWARE.md.
- Guard:
  - independent clock and state;
  - the inverted shadow detects corruption and re-arms the lockout;
  - Y1 cannot start before 5 min (boot included);
  - O cannot change while Y1 runs or within 5 min of it stopping;
  - W is blocked with Y1+O;
  - G is forced on with Y1 or W;
  - a NULL request means all off;
  - pins switch offs first and Y1 last.
- hvac_logic:
  - min-off is armed at boot;
  - the refused-start and unexpected-stop feedback through `hvac_reconcile` is correct;
  - hysteresis;
  - auto changeover waits 10 min idle (from e-heat too);
  - aux staging only on top of Y1;
  - e-heat never runs Y1;
  - the fault path drops Y1, G and W with no purge;
  - inputs are sanitised;
  - the `_Static_assert`s on setpoint ranges hold.
- Clocks: `esp_timer` is 64-bit ms everywhere, and every elapsed-time calculation guards against time going backwards. No wrap is possible.
- Watchdogs:
  - the task WDT panics after 5 s (`CONFIG_ESP_TASK_WDT_PANIC=y`) and the IDLE task is watched;
  - panic → reboot with no delay;
  - brown-out detection is on;
  - all of these resets drop the pins.
- Settings:
  - the 16-byte blob layout is pinned by `_Static_assert`;
  - a size or version mismatch gives the defaults (mode Off);
  - each out-of-range field resets to its default, and a broken setpoint pair resets as a pair;
  - the commit is deferred, with the NVS write outside the lock.
- Sensor:
  - both CRCs are checked (all-0x00 and all-0xFF frames fail the CRC);
  - the conversion fits `int16_t`;
  - the 2 min fault rule works;
  - the self-heat lag is integer-only and reaches its target without overshoot;
  - a missing SHT40 does not abort start-up.
- I2C: the IDF `i2c_master` bus lock serialises the OLED (UI task) and the SHT40 (control task). A missing or stuck panel causes timeouts, not hangs; the worst case is a sensor fault, which is fail-safe.
- Matter bridge:
  - PRE_UPDATE vetoes unsupported SystemModes and POST_UPDATE commits;
  - setpoint snapping in °F (verified to ±0.25 °C);
  - the half-°C snap followed by `hvac_setpoints_apply` clamps 599/761 back into range;
  - `report()` does not re-enter the callbacks (`call_callbacks=false`);
  - Thread is configured before `esp_matter::start`;
  - a Matter failure leaves the thermostat running locally.
- `bridge_map` converts with rounding and saturation. The running-state bits are right, and the fan mapping matches sequence 4 (Off/High/Auto).
- UI and display:
  - D-pad debounce and repeat;
  - `pages_nav` setpoint keys go through `hvac_setpoints_apply`;
  - dim and wake;
  - the burn-in creep keeps all content inside the usable area (largest x 125+3 and y 61+2);
  - drawing happens only on the main task.
- Size: `check_size.py` works on the current build (82.4% flash against a 90% budget, 40.7% static RAM).
- Partitions: two 0x1E0000 OTA slots, and the app image goes at 0x20000, as the README says.
- The factory reset (`matter esp factoryreset`) clears only the CHIP namespaces, the KVS and the Thread data, so the "thermo" settings survive, as the README says.
- `cad/fusion_case_v3.py` (light pass): the XIAO C6, relay module, XL7015, JST-PH 9-pin and SHT40 boxes are consistent with HARDWARE.md.
