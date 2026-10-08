# Firmware review, Oct 2026 — Sympathizer

Scope: the whole firmware code base after the removal of ESP32-S3 support (components/, matter/, tools/, test/, docs). Read-only review; `python tools/check_safety_sources.py` run: **"relay path OK"**. No builds or `pio test` were run.

## Summary

The safety design holds up well. The two layers really are independent:

- `hvac_logic` is pure and fully host-tested.
- `relays_guard` has its own state, its own clock read and a bit-inverted shadow copy.

Each layer is enough on its own to stop the most damaging failure, a compressor short-cycle or a valve flip under load. The physical-world fallbacks are real: reset leaves pins low, there are 10 k pull-downs, and the watchdog and brown-out paths lead to a reset. Most choices that look odd at first are deliberate and documented, and several show careful thinking:

- int16 tenths °F as the storage unit;
- the single-rounding setpoint snap;
- committing only in POST_UPDATE;
- re-writing the setpoint limits at every boot;
- the 1 s minimum-run margin.

The genuine issues are minor; none breaks the minimum-off or valve rules. The most notable:

- **S1, a rule-precedence conflict.** On a sensor fault the guard keeps O energised for up to 5 min. This literally breaks "sensor fault, all outputs off", although it is physically harmless.
- **S2, an off-by-0.01 °C mismatch.** The reported setpoints and the advertised Matter limits differ by 0.01 °C at the range ends.
- A settings read-modify-write race, a fail-open RAM check, an incomplete strapping-pin list, and stale documentation.

## 1. The safety argument, rule by rule

Columns: **L** = `hvac_logic` (computes the request), **G** = `relays_guard` (filters the request, `relays_guard.c`), **HW** = hardware or reset.

| Rule | L | G | HW / reset | Independent stops |
|---|---|---|---|---|
| Y1 min-off 5 min, armed at boot | `hvac_drive_compressor` `b_off_ok`, hvac_logic.c:278-298; `hvac_init` arms it, :484 | relays_guard.c:91-102; armed in `relays_init`, relays.c:53 | Reset drops pins (relays.c:40-50), and both layers re-arm | **2 layers** + outdoor-unit delay (CLAUDE.md:56-58) |
| O changes only after Y1 has been off ≥ 5 min | hvac_logic.c:282-289 (O and Y1 wait for the valve) | relays_guard.c:94-98: O is held **and Y1 is dropped**, so Y1 never runs against the wrong valve | — | **2 layers** |
| Min run 3 min | `hvac_min_run_holds` with 181 s hold, hvac_logic.c:248-253, hvac_logic.h:56-62 | Not enforced by design: "off is always allowed", so a fault can always stop Y1 | — | 1 layer (by design) |
| W never with Y1 + O (strips while cooling) | COOL forces W = 0, hvac_logic.c:369-371; W only on top of heating Y1, :335-357 | relays_guard.c:103-106 | — | **2 layers** |
| Y1 or W imply G | hvac_logic.c:391-392 | relays_guard.c:107-110 | — | **2 layers** |
| E-heat = G + W, Y1 always off | hvac_logic.c:373-383 | The guard does not know the mode | — | 1 layer |
| Sensor fault → all off | `hvac_fault`, hvac_logic.c:433-445; fault after 2 min, sensor_logic.c:216-227 | Passes offs, **except O** (see S1) | — | 1 layer (plus S1 caveat) |
| Watchdog or brown-out → all off | — | — | `CONFIG_ESP_TASK_WDT_PANIC=y`, 5 s; `CONFIG_ESP_BROWNOUT_DET=y` (sdkconfig.defaults:9-13). Reset leaves pins as inputs; pull-downs hold them | HW |
| Pins low first | `ESP_ERROR_CHECK(relays_init())` is the first statement of `app_start` (app.c:82). The latch is written low **before** the output driver is enabled, then written again after (relays.c:40-50) | — | 10 k pull-downs (HARDWARE.md:68) | HW + FW |
| No relay pin on a strapping or UART TX pin | board.h:32-35 uses GPIO 1, 2, 21, 18; checked by `check_safety_sources.py` (rule 5) | — | — | Tool (see S5 for the gaps in its list) |

### Supporting mechanisms that make the two layers agree instead of fight

- **Feedback** (hvac_logic.h:37-44, `hvac_reconcile` hvac_logic.c:398-430; control.c:115-121). The logic adopts what the guard actually drove.
  - A refused start restores the old off-edge, so it is asked for again next second.
  - An unrequested stop re-arms min-off in full.
  - Without this, the guard reading the clock a few ms later than the logic would cause a 1-cycle refusal. The logic would then time a run that never happened.
  - test_safety_chain injects 0–49 ms of skew to prove this (test_safety_chain:30, 116).
- **Corruption.**
  - Both state words have an inverted copy (relays_guard.c:28-45).
  - A mismatch forces all off and restarts min-off "from now" (:80-87). That is a reset in place.
  - It is fuzzed with random bit flips over 4 M steps (test_relays_guard fuzz_corrupt, FUZZ_SEEDS × FUZZ_STEPS = 16 × 250 000).
- **Backwards clock.** It fails safe in both timer layers:
  - `hvac_elapsed` returns 0 (hvac_logic.c:32-42);
  - the guard needs `now_ms > off_ms` (relays_guard.c:91).

  A forward jump would fool both, because both read esp_timer. This is the residual gap, and CLAUDE.md:56-58 already gives its hardware answer (delay-on-break, or the outdoor unit's own delay).
- **Pin reach.** Every pin write goes through the guard:
  - only relays.c names relay pins or calls `relays_guard_step`;
  - only control.c calls `relays_apply` (check_safety_sources.py:39-53, passes today).
  - Within one apply, the order is: offs first, then O and G, then W, and Y1 last (relays.c:86-104).
- **Boot and NVS.**
  - A missing, wrong-sized or wrong-version blob gives the defaults with mode **Off** (settings.c:49-57, settings_logic.c:52).
  - Each field is range-checked, and a pair that breaks the deadband is reset together (settings_logic.c:74-79).
  - NVS itself CRCs each entry.
  - `hvac_sanitise` re-clamps mode, fan and setpoints on every step anyway (hvac_logic.c:126-147). Bad NVS data therefore has to pass two independent sanitisers before it reaches the logic.
- **Start-up order.**
  - relays, then the RF switch, then NVS and settings, I2C, display, sensor, D-pad, control.
  - **Only then** is Matter started (app.c:82-106), so a slow or failing network stack can never delay or starve the control loop.
  - Start-up failures abort (D3), and the abort leads to the safe reset state.

### Residual gaps (honest)

1. The two hardware gaps CLAUDE.md lists: float before `relays_init`, and the lack of a true hardware timer.
2. **Fault → all off** is single-layer, and at the pins O can stay on (S1).
3. **E-heat Y1-off** and **minimum run** are single-layer.
   - A guard cannot sensibly enforce min-run, because then a fault could not stop the compressor.
   - The guard does not know the mode.
4. **A forward jump of esp_timer** defeats both layers, because both read the same clock. Only the hardware delay covers it.
5. **No plausibility check on a CRC-valid reading** (S7).

## 2. Likely criticisms and why they do not hold

**C1. "The guard is not independent: it uses the same esp_timer clock."**

Its independence is of **inputs and state**, not of oscillator:

- it reads the clock itself (relays.c:66) instead of taking the control task's `now_ms`;
- it keeps its own off-edge and its own "applied" record;
- it shares no code with hvac_logic (relays_guard.h:6-8).

That defeats every software failure in the control path: a bad `now_ms`, corrupted `g_hvac`, or a logic bug. A clock fault is the stated residual gap, and backwards time fails safe in both layers.

**C2. "The guard doesn't enforce minimum run, e-heat Y1-off, or fault-off."**

This is intentional. A filter that can refuse an *off* would block the fault path. The guard only removes energy (Y1, W, an O change) or adds G, so it can never make a state less safe. The single-layer rules are covered by unit tests:

- e-heat: test_hvac_logic mixed run, lines ~790-830;
- faults: :415-456;
- minimum run: test_safety_chain:89-95, on the pins' clock.

**C3. "Minimum run is waived on Heat→Cool and →e-heat, against the hard rule."**

It is documented (hvac_logic.h:23-25), and CONTROL_SPEC:22 already allows "mode-off overrides". "The mode no longer allows this call" is the same principle. The alternatives are worse:

- Heating for up to 3 min after the user selected Cool.
- Running a compressor the user has just declared unusable by choosing e-heat.

The harm is bounded: every start still needs 5 min off, so mode toggling cannot raise the start rate above about 12 per hour. *Recommend aligning CLAUDE.md:70 with the header.*

**C4. "O and Y1 switch in the same cycle."**

This happens only once min-off has been met (both layers). O is written before Y1 (relays.c:95-104). A reversing valve needs compressor pressure to shift anyway, and conventional thermostats energise O with or before Y.

**C5. "Relay pins float or glitch at boot."**

Several layers handle this:

- The output latch is set low before the driver is enabled (relays.c:38-45). There is no high glitch on the way to becoming an output.
- `relays_init` is the first statement in `app_start`, and 10 k pull-downs are specified.
- Even if a C6 pad had its weak internal pull-up (~45 kΩ) at reset, it could source only about 50 µA into an opto LED. An opto needs mA to switch a relay, so it cannot energise a coil.
- A reset loop never starts the compressor, because each boot re-arms the 5 min lockout.

**C6. "int16 tenths °F loses precision against Matter's 0.01 °C."**

- 0.1 °F is 0.056 °C, finer than the SHT40's useful accuracy.
- Both of the user's grids are **exact** in tenths °F:
  - whole °F;
  - half °C, since 0.5 °C = 0.9 °F (PAGES_STEP_SP_C_F10 = 9, pages_nav.h:51).
- Conversion happens at exactly one boundary (bridge_map.h:7-9).
- The whole-°F round trip is tested from 40 to 99 °F (test_bridge_map:30-41).

The control path never sees Celsius.

**C7. "Snapping Matter writes to whole °F changes the user's value."**

Snapping recovers it:

- The Home app sends Celsius it rounded itself, so 65 °F arrives as 65.1 or 65.2 °F (bridge_map.h:90-92).
- The snap rounds **once**, straight from c100 to whole °F (bridge_map.c:77-87), which avoids double-rounding drift. This is tested for every degree from 60 to 80 with ±0.25 °C of error (test_bridge_map:54-64).
- In Celsius display mode it snaps to the device's own 0.5 °C step.
- Any value pushed out of range by the snap is clamped by `hvac_setpoints_apply` (matter_bridge.cpp:96, 103), and `settings_set` sanitises again.

**C8. "Acting only in POST_UPDATE, and refusing in PRE_UPDATE, is fragile."**

- POST_UPDATE fires only after the cluster has validated and stored the write (limits, deadband). Settings therefore never take a value Matter then rejects.
- PRE_UPDATE does one job: refusing system modes the thermostat cannot run (matter_bridge.cpp:162-172).
- The `memcmp` guard (:196) stops redundant `settings_set` calls, flash writes and feedback loops.
- The sync task uses `attribute::report` (not `update`), which does not re-enter the app's attribute callback.

**C9. "Writing all eight limit attributes every boot is wasteful."**

It is necessary: Matter restores non-volatile limits saved by older firmware over the values passed to `create()` (matter_bridge.cpp:238-255). Without the set, a changed range would never take effect. The cost is eight int16 writes per boot. `bridge_report_limits` (:455-484) then tells subscribers who read the limits at pairing.

**C10. "E-heat as an On/Off 'outlet' is a hack."**

- Apple Home has no EmergencyHeat SystemMode.
- While e-heat is on, a write of Heat keeps e-heat (bridge_map.c:158-163). Home habitually re-writes Heat, which would otherwise silently cancel e-heat.
- Any other mode cancels it, and so does the switch.
- Every case is tested (test_bridge_map:118-138).

**C11. "Tests #include the .c (D4)."**

This keeps the native build free of ESP-IDF and lets the tests reach `static` functions and state. It is documented as D4, test-only, and enforced by convention ("production never includes a .c").

**C12. "The C++ bridge is exempt from the strict flags (D9)."**

- The connectedhomeip headers do not build under `-Wconversion -Werror`.
- All arithmetic has been moved into the strict, host-tested `bridge_map.c` (CMakeLists:449-450).
- The bridge holds **no safety logic**. Matter only calls `settings_set`, exactly like a key press, and the result passes `settings_logic_sanitise`, then `hvac_sanitise`, then the guard.

**C13. "Silent assertions, no esp_err name table, and hex error logs reduce safety and debuggability."**

- A failed assertion still aborts, and the abort resets the chip, which drops the relays. Only the strings are removed (sdkconfig.defaults:17-23).
- The code consistently logs `0x%x` and never calls `esp_err_to_name` (grep finds none), so nothing breaks.
- DHCPS is off because there is no softAP. OpenThread diag and CLI are off because nothing uses them. All are pure flash savings on a 4 MB part.

**C14. "A 90% flash budget is too high."**

- The check runs against the **smallest** OTA slot (check_size.py:39-49), so OTA stays viable.
- Matter, Thread and BLE are fixed costs. The 90% figure is explicit and has a revisit note (matter.sh:69-72).
- The runtime heap low-water mark is logged (control.c:143-166). That covers the heap the static check cannot see.

**C15. "u8g2 is fetched from the network."**

It is pinned to a tag and cloned once (matter.sh:44-50). The build fails loudly if the sources are missing (components/u8g2/CMakeLists.txt:12-17). This avoids vendoring about 10 MB of fonts.

**C16. "The shared I2C bus has no project lock."**

- ESP-IDF's `i2c_master` serialises per bus (i2c_bus.h:6-8).
- Each transaction carries a timeout: 50 ms for the sensor (sensor.c:26) and 100 ms for the OLED (display_hal.c:18). In IDF 5.x that timeout also bounds the wait for the bus lock.
- A missed sensor read costs nothing until 2 min of misses.
- Only the UI task touches the OLED and only the control task touches the SHT40, so there is no cross-task use of a device handle.

**C17. "The burn-in shift mutates the frame buffer in place."**

- Every frame is cleared first (pages.c:390), then shifted, then sent (display.c:95-106). Nothing accumulates.
- Layouts are kept inside `DISPLAY_USABLE_W/H` (pages.c:28-31).
- The shift is pure and host-tested.

**C18. "The 1 s hold margin is arbitrary."**

- It is exactly one control period, which bounds the varying delay between the decision and the pin write (hvac_logic.h:56-62).
- test_safety_chain checks the 3 min run on the **guard's** (pins') clock.
- check_relay_log.py checks it on the real log.

**C19. "The control logic uses a stale temperature for up to 2 min."**

That is the spec (CONTROL_SPEC:32). Any call is still bounded by hysteresis, the setpoint limits (60–80 °F) and the timers.

**C20. "Matter or Thread could starve the control task."**

- Control runs at priority 5 and starts before Matter (app.c:100-106).
- Its stack is static, and its mutex is static (`xSemaphoreCreateMutexStatic`), so it needs no runtime heap.
- If it is ever starved for 5 s, the task watchdog panics, the chip resets and the pins go low.

## 3. Concurrency map

| Shared state | Writers | Readers | Protection |
|---|---|---|---|
| `g_cfg` (settings.c) | UI task (`app_ui_keys`), CHIP task (`bridge_attribute_cb`) | control, UI, matter_sync, CHIP | Static mutex; whole-struct snapshots (rule 9). **Read-modify-write is not atomic across get and set** (S3) |
| `g_status` (control.c) | control task | UI, matter_sync | Static mutex (control.c:136-140, 217-221); NULL lock checked |
| `g_hvac`, `g_guard`, `g_reading`, `g_logic` | control task only (init on main before the task exists) | control task only. `relays_applied` and `relays_energised_count` are called only from control, and from sensor_sim inside the control task | Ownership |
| OLED, u8g2 buffer | main/UI task only (start-up screens on the same task) | — | Ownership (rule 9) |
| I2C bus | UI (OLED handle), control (SHT40 handle) | — | `i2c_master` bus lock with per-transaction timeouts |
| Matter attributes | CHIP task, matter_sync (`attribute::report`) | — | esp-matter takes the CHIP stack lock in report/update (not verified against the v1.6 source here: it is not accessible from Windows) |
| `g_reported` (bridge) | matter_sync only | matter_sync only | Ownership |

## 4. Tests, tooling, docs

### What the tests prove

- **test_safety_chain.** Real `hvac_logic` and `relays_guard`, wired as in control.c (feedback included), checked on the guard's clock.
  - Run: 12 seeds × 400 k one-second steps, 0–49 ms skew, random resets, faults and mode changes.
  - Checked: min-off (resets counted as an off), the O rule, W/Y1/O, G, e-heat Y1-off, minimum run unless waived. Liveness is checked too (more than 10 starts).
  - **Not checked:** the order of pin writes inside relays.c; "all off at the pins on a fault" (this is how S1 slipped through); skew above 50 ms. The real-hardware check of jitter is check_relay_log.py.
- **test_relays_guard.** Each rule directly; corruption of every state word; 4 M fuzzed hostile requests with random dt and bit flips. Its assertion "off is never refused" covers only Y1 and W, not O. That matches S1.
- **test_hvac_logic.** 49 cases plus a 40 k-step invariant run. The invariant exempts O changes on a fault (lines ~795-798), so the fault/O precedence is known in this layer.
- **test_bridge_map.** Conversions, snapping, modes, fan, running state, and the limits against Matter's gap rule. **Missing:** that the setpoints at the range ends, once converted, lie inside the advertised limits (S2).

### Tooling

- **check_safety_sources.py** enforces what it claims for the named APIs. It is a lint, not a proof: register writes, `gpio_ll_*` and raw integer pin numbers would slip past it. The C6 strapping list is incomplete (S5).
- **check_size.py** checks flash against the smallest slot correctly. Its RAM check fails open if the `esp_idf_size` JSON schema has no `diram`/`dram` keys (S4).
- **check_relay_log.py** mirrors the rules on the log's own timestamps, and treats each boot as a re-arm.

### Docs after the S3 removal

README.md, matter/README.md, HARDWARE.md, platformio.ini, test/README and CLAUDE.md all describe a C6-only build. **No S3 references remain** outside third-party `.pio/` caches and old review files. Staleness that remains is listed under S9.

## 5. Genuine issues found

**S1. Medium-low. On a fault, O can stay energised for up to 5 min. The two layers disagree on precedence.**

- **Where:** relays_guard.c:94-98 against hvac_logic.c:441, and against the guard's own claims at relays_guard.h:8 ("Turning anything off is always allowed") and relays.h:32 ("NULL drives everything off").
- **Scenario:**
  1. Cooling (Y1, G, O on) when the SHT40 times out for 2 min.
  2. `hvac_fault` requests 0 0 0 0. The guard sees an O change while Y1 is still applied, so `b_off_ok` is false. It holds O = 1 and drops Y1 and G.
  3. The pins stay at O-only for 5 min while the OLED shows SENSOR FAULT.
  4. relays.c:78-84 logs a "guard changed request" WARN every second, about 300 lines.
- **Severity:** physically benign, because a valve solenoid alone with the compressor off does nothing, and O is held indefinitely in Off by the spec anyway. It is still a literal breach of the hard rule "sensor fault → all outputs off", and no test checks fault → all off at the pins.
- **Fix:** pick one precedence.
  - Either the guard lets O go to 0 when the whole request is all-off and Y1 is stopping (the same as a reset or a corruption resync, which already drop O at once, relays_guard.c:80-87),
  - or amend CLAUDE.md, CONTROL_SPEC and the two header comments to "all off except O, which the valve rule holds".
  - Add a test at the pin level either way.

**S2. Low. The reported setpoints at the range ends fall outside the advertised Matter limits by 0.01 °C.**

- **Where:** bridge_map.c:100-106 (limits nudged inward by ±1) against matter_bridge.cpp:387-388 and 411-421 (reports not clamped).
- **Arithmetic (verified):**

  | Setpoint | Reported (c100) | Advertised limit (c100) |
  |---|---|---|
  | 60 °F | 1556 | min 1557 |
  | 76 °F (heat) | 2444 | max 2443 |
  | 64 °F (cool) | 1778 | min 1779 |
  | 80 °F | 2667 | max 2666 |

- **How it is reached:** the D-pad can set any of these ends, and a Home write of 1557 snaps to 60 °F, which is reported as 1556.
- **Effect:** depends on esp-matter's bounds handling, which was not verified here.
  - The report may be refused: it is logged, and Home keeps a stale value.
  - Or an out-of-limit value goes out to controllers.
- **Fix:** drop the ±1. The exact values 1556, 2444, 1778 and 2667 convert back to exactly 600, 760, 640 and 800, and they still meet the Matter deadband (1556 + 170 ≤ 1778; 2444 + 170 ≤ 2667). Alternatively, clamp the reported values into the limits. Add a test.

**S3. Low. Settings read-modify-write race: a lost update.**

- **Where:** app_ui.c:78-83 against matter_bridge.cpp:159-200.
- **Scenario:**
  1. The UI task (priority 1) does `settings_get`.
  2. The CHIP task preempts it and sets mode = Cool from Home.
  3. The UI calls `settings_set` with its stale copy (mode = Heat and a new setpoint).
  4. Home's mode change is lost. The sync task reports Heat back within 1 s, and the user sees it revert.
- **Severity:** not a safety issue, since every write is sanitised. The window is microseconds.
- **Fix:** a `settings_update(fn, ctx)` that applies the edit under the lock, or per-field setters.

**S4. Low (tooling). check_size.py passes the RAM check when it cannot measure.**

- **Where:** check_size.py:63 and :87.
- **Scenario:** a different `esp_idf_size` JSON schema returns `(0, 0, "none")`. `ram_ratio` becomes 0.0, so the result is "OK".
- **Fix:** exit 1 when `region == "none"`.

**S5. Low (tooling and docs). The C6 strapping list omits GPIO4 (MTMS) and GPIO5 (MTDI).**

- **Where:** check_safety_sources.py:56-59, HARDWARE.md:65, board.h:27.
- **Severity:** no current violation, since the relays are on 1, 2, 21 and 18. A future re-pin onto 4 or 5 would pass the check, though.
- **Fix:** add 4 and 5 (verify against the ESP32-C6 datasheet's strapping table). Consider forbidding 12 and 13 (USB D−/D+) and 17 as well.

**S6. Low (hardware). The right key is on U0TXD (GPIO16) and may short a driven output.**

- **Where:** board.h:43.
- **Scenario:** from reset until `buttons_init`, GPIO16 is probably still the ROM's push-pull UART TX, idling high. Holding the right key in that window (under 1 s) shorts it to GND. board.h:28-30 says this "only garbles the ROM's boot message"; it may also briefly overload the pin.
- **Not verified:** whether the IDF bootloader leaves TX driven when the console is USB-JTAG.
- **Fix:** a 1 k series resistor in that key line.

**S7. Low. Any CRC-valid SHT40 frame is accepted as valid, with no plausibility window.**

- **Where:** sensor_logic.c:90-134, with sensor_logic.c:222 also failing open on `now < last_good` (unreachable today).
- **Scenario:** a damaged sensor or a mis-read with a good CRC reports 150 °F. The result is a cool call in Cool or Auto, and no fault is ever raised.
- **Fix:** treat readings outside about 32–110 °F as failed reads, so the existing 2 min fault rule takes over.

**S8. Low. The control task only logs a failed watchdog subscription.**

- **Where:** control.c:176-180.
- **Severity:** with `CONFIG_ESP_TASK_WDT_INIT=y` this cannot fail in practice. If it ever did, the loop would run without hang protection.
- **Fix:** abort (reset) when the subscription fails.

**S9. Low (docs). Stale or contradictory text.**

- **CLAUDE.md:**
  - :42 says "tenths of °F or °C"; storage is always °F.
  - :45 says the commissioning QR is shown on the OLED; :97 says it is not started.
  - :44 mentions "a migration"; none exists yet.
  - :70 states minimum run unconditionally; compare hvac_logic.h:23-25 (see C3).
- **CODING_STANDARD.md** still carries leftovers from the NTP desk clock:
  - D1 mentions test_weather and cJSON;
  - D3's rationale mentions "the clock … network stack";
  - D5 mentions settings_defaults.h and the recovery AP;
  - rule 2 cites `net_bring_up_stack()`;
  - rule 9 cites `app_events` bits, which do not exist.
- **Wi-Fi leftovers on a Thread-only build:** matter_bridge.cpp:575-577 and sdkconfig.defaults:57 mention Wi-Fi shell commands.
- **settings.c:100-105** says "a later real build still starts from Off". That is false if any setting was changed on the sim build, because an `-app.bin` update keeps NVS.
