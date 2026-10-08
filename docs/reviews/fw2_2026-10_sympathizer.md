# Firmware review 2, Oct 2026: sympathizer (safety and compatibility)

Scope: the tree as of 2026-10-07, after the fixes from `fw_2026-10_synthesis.md`. I read the code only. No builds, no `pio test`. `python tools/check_safety_sources.py` prints `relay path OK`. Upstream behaviour was checked in the WSL sources: esp-matter `release/v1.6` @ c6607128 (17 Sep 2026), connectedhomeip @ 93abd8e6, ESP-IDF `v5.5.5` (`git describe`). The resolved sdkconfig is `~/build/openthermo-matter-esp32c6/sdkconfig`.

Line numbers are for the current files.

---

## 1. Safety

### 1.1 Each hard rule: which layers enforce it, and which test proves it

| Hard rule (CLAUDE.md) | hvac_logic | relays_guard (own clock, `relays.c:66`) | Other | Proof |
|---|---|---|---|---|
| Heat = Y1+G | `hvac_logic.c:403` (G from Y1/W) | G forced with Y1 or W, `relays_guard.c:121-124` | | `test_hvac_logic` (heat cases); `test_relays_guard:test_g_forced_with_y1_or_w`; chain check `test_safety_chain/test_main.c:116` |
| Cool = O+Y1+G | `hvac_drive_compressor` `:279-304`: Y1 never runs with the wrong O | Y1 held off while an O change is refused, `relays_guard.c:97-112` | | `test_cool_sets_o_with_y1`, `test_o_change_refused_while_y1_runs` |
| Aux = add W | W only on top of a running Y1, `:340-362`; never in cool `:381-384`; dropped in a held-over heat run during Cool `:371-380` | W+Y1+O refused, `relays_guard.c:117-120` | | `test_aux_never_in_cool`, `test_heat_run_held_into_cool_drops_aux`, `test_aux_refused_while_cooling`, chain `:117` |
| E-heat = G+W, Y1 always off | `:385-395` | none (the guard has no notion of mode) | | `test_eheat_*`, chain `:118-121` (Y1 never on in EHEAT mode). Single layer, by design. |
| Fan = G | `:403` | | | `test_fan_on_runs_g_when_idle` |
| Min off 5 min, also at boot | `:507` (boot), `:279-304`, reconcile `:410-442` | `relays_guard.c:92-93, 113-116`; boot `relays.c:53` | both re-arm on reset | `test_heat_at_boot_waits_five_minutes`, `test_y1_refused_until_min_off_after_boot`, chain `:86-88` across random resets |
| Min run 3 min | `hvac_min_run_holds` `:250-259`, held one cycle extra (`HVAC_MIN_RUN_HOLD_MS`) | deliberately not enforced (README "Safety": a guard that can refuse an off could block the fault path) | | `test_min_run_*`, `test_*_holds_the_run_then_waits`; chain `:93-98` on the guard's clock with jitter |
| O changes only after Y1 has been off ≥ min-off | `:287-303` | `relays_guard.c:97-112` | | `test_o_changes_only_after_min_off`, `test_o_change_refused_until_min_off`; chain `:101-111` |
| Fault → all off | `hvac_fault` `:449-468` (O included, min-off re-armed) | an all-off request is honoured in full and re-arms, `:95, 97-106`; state corruption → all off `:81-88` | TWDT 5 s panic, INT WDT 300 ms, brown-out level 7, bootloader WDT 9 s (resolved sdkconfig) | `test_fault_*`, `test_all_off_drops_o_and_rearms`, `test_corrupted_*`; chain `:112-115` (every output off on every fault step) |
| H-trigger; pins low as early as possible | | latch written low **before** `gpio_config`, then again after (`relays.c:40-50`); first call in `app_start` (`app.c:82`) | 10 k pull-downs for the ROM/bootloader window (HARDWARE.md:68) | review |
| No strapping or UART-TX relay pin | | | `check_safety_sources.py:63-66` (C6: 8, 9, 15, 16); relays on 1, 2, 21, 18 | lint run before every build/test. The list is incomplete; see S2. |
| Only relays.c reaches the pins | | | `check_safety_sources.py:43-55` | lint, run here: OK |

### 1.2 Can the all-off exemption be reached from a non-fault path? No.

The exemption (`relays_guard.c:97-106`) fires only when two things are true together:
- the request is all four outputs off;
- it changes O inside min-off.

hvac_logic starts every step from what was really driven (`hvac_reconcile` `:441`, fed by `control.c:115-120`). So it holds O unless one of these happens:
- **Heat or cool call.** O changes only when `b_off_ok` is true (`:287-291`), and then `p_out->b_y1 = b_off_ok = true` (`:300-303`). Y1 is requested in the same step, so the request is not all-off.
- **E-heat.** O drops only with `out.b_w = true` (`:387-394`).
- **No call or Off.** O is held (`:396-400`).

That leaves two sources of an O-dropping all-off request: `hvac_fault`'s `memset` (`:464`) and `relays_apply(NULL)`. Both are fault paths by contract.

Clock skew doesn't change this. If the logic's `b_off_ok` is true and the guard's is still false, the guard refuses the O change and Y1 together, but G stays requested. That is the non-exempt branch, `:107-111`.

`test_safety_chain/test_main.c:101-111` checks this empirically. Every O change on the driven pins that the valve rule doesn't allow must have `b_temp_valid == false`. It runs 12 seeds × 400k steps, with resets, 1–400 s faults, mode changes and guard jitter.

On the equipment side, the exemption drops O with the compressor stopped or stopping. A power cut or a reset does exactly the same, so the valve and compressor must already tolerate it. Min-off restarts at that instant, so the next Y1 start still waits 5 min.

### 1.3 Can the min-run hold keep Y1 on when it must stop? No.

`hvac_min_run_holds` (`:250-259`) needs all of these:
- the logic's own `out.b_y1`, which after reconcile is the *driven* Y1;
- a HEAT or COOL call;
- a mode other than OFF or EHEAT;
- elapsed time under 181 s.

Each case where Y1 must stop:
- **Fault.** Handled before demand (`:544-546`), so the hold is never consulted.
- **Off or e-heat.** Excluded explicitly.
- **Guard dropped Y1** (corruption or a refused start). Reconcile adopts `b_y1 = false`, so the hold no longer applies, and `drive_compressor` can't restart Y1 until min-off has passed (`:284-303`).
- **Clock going backwards.** `hvac_elapsed` returns 0, so a hold could last longer. But esp_timer is monotonic, and the test `test_clock_going_backwards_does_not_start_y1` covers the start side.

What the hold can do is keep heating for up to 3 min after a switch to Cool, or the reverse. The spec mandates this (CONTROL_SPEC "Compressor minimum run"). The strips are dropped (`:371-380`), and the guard's W+Y1+O rule backs this up for the cool direction.

### 1.4 Can the callback in `settings_update` starve the 1 s control task (5 s watchdog)? No.

- **Lock duration.** The edit callbacks (`app_ui_key_edit` → `pages_nav_key`, pure; `bridge_edit`, pure plus `std::atomic`) run in microseconds (`settings.c:164-177`).
- **Priority inversion.** The mutex is a FreeRTOS mutex (`xSemaphoreCreateMutexStatic`, `settings.c:100`), so it has priority inheritance. A priority-1 UI or CHIP holder is boosted to 5 while control waits.
- **Flash writes.** The NVS write runs **outside** the lock (`settings.c:194-206`).
- **Deadlock.** No path nests locks. The control task takes the settings lock, then its own lock, sequentially (`control.c:98, 138`), and never takes the CHIP lock. `bridge_sync` copies settings and then status, sequentially (`matter_bridge.cpp:479-480`). The CHIP task holds the stack lock while it takes the settings lock, and nobody takes them the other way round.
- **Bus waits.** The I2C waits are bounded (sensor 50 ms, OLED 100 ms).
- **Worst case.** A buggy callback that never returns blocks control, the TWDT panics, the chip resets, and every relay is off.

### 1.5 Panel-off and migration cannot affect the outputs

- **Panel-off.** `app_ui_power` (`app_ui.c:117-139`) only calls `display_power`, on the UI task. While the panel is off, the first key only wakes it (`app_ui.c:85-96`), so a blind press can't change the mode.
- **Migration.** `settings_logic_from_blob` (`settings_logic.c:140-189`) copies every v1 field and defaults `screen_off_s`. `settings_init` then sanitises (`settings.c:126`), and an invalid mode falls back to Off. Mode and setpoints carry over unchanged.
- **Downgrade.** An older image reading the 18-byte v2 blob into a 16-byte buffer gets `ESP_ERR_NVS_INVALID_LENGTH` (IDF `nvs_api.cpp:521-524`), so it uses the defaults, whose mode is **Off**. That fails safe.

### 1.6 Boot, reset, brown-out and watchdog: what the house sees

- **Reset to `relays_init`.** The pins are inputs; the 10 k pull-downs hold IN low (the residual gap CLAUDE.md lists). The bootloader WDT (9 s) bounds a hung boot.
- **Boot.** Every IN is low before the drivers are enabled (`relays.c:38-50`). Both layers arm min-off (`relays.c:53`, `hvac_logic.c:507`). Matter starts only after the control loop (`app.c:100-107`), so a slow Thread join can't delay control.
- **Control task hang.** The TWDT (`CONFIG_ESP_TASK_WDT_PANIC=y`, 5 s, idle task also checked) panics. `PANIC_PRINT_REBOOT` with delay 0, and IDF arms the RTC WDT inside the panic handler (`panic.c:203-212`), resets the chip, and the pins drop. **The relays stay energised for at most about 5 s, then everything is off, followed by a 5 min compressor lockout.**
- **Interrupts disabled.** INT WDT at 300 ms.
- **Brown-out.** Detector at level 7, ISR then restart. The relay coils run from the same 24 VAC, so they sag too.
- **Another task hung.** The outputs stay correct; only Matter or the UI are affected.
- **Start-up errors.** `ESP_ERROR_CHECK` aborts and resets with the outputs low. A missing OLED or SHT40 doesn't abort: `display_init` and `sensor_init` only register devices, so there is no boot loop from a dead peripheral.

Residual gaps, already listed in CLAUDE.md:
- the pins float before `relays_init`, which the pull-downs cover;
- no hardware delay-on-break, so rely on the outdoor unit's ASC.

Two more worth stating:
- **One sensor feeds the loop.** A CRC-valid but wrong reading isn't caught; see S1.
- **"All off" means no heat in winter.** That is the right failure for a relay fault, but a crash loop means a cold house. The Braeburn is the fallback.

---

## 2. Compatibility

### 2.1 HVAC equipment

**Compatible:**
- Single-stage heat pump (Y1 only) with an **O** reversing valve (energised in cool).
- Electric strips on one W terminal, used as both aux and emergency (W1/E jumpered to W2, HARDWARE.md:14).
- One 24 VAC transformer (Rh jumpered to Rc).
- A C wire (it powers the board, HARDWARE.md:21-23).
- The firmware hard-codes all of this: `hvac_drive_compressor(…, b_want_o = true for cool)` `:381-383`; one W.

**Relay ratings.** The SRD contacts (10 A) are far above the 24 VAC loads; contactor pickup is about 1.25 A per HARDWARE.md:30. The common R feed is fused T1.6 A (F2).

**Not compatible**, and the docs never say so in one place (see S4):
- **B-type valves** (energised in heat; Rheem/Ruud). This firmware would cool when calling heat. Nothing selects O or B.
- **Two-stage compressors (Y2) or separate W1/W2/E staging.**
- **Dual-fuel heat pumps (gas furnace backup).** Y1+W together over a furnace is wrong. So are e-heat as "strips" and the 3 °F aux rule.
- **Conventional furnace or AC systems** with no reversing valve.
- **Split Rh/Rc (two transformers).** COM1..4 are jumpered to one R (HARDWARE.md:25), so this would bridge the transformers.
- **No C wire.**

**Braeburn 1220NC scope.** The 1220NC is a 2H/1C heat-pump stat with installer-selectable O/B. This firmware replaces it only in its O, single-stage, electric-aux configuration, which is what the wall wiring in HARDWARE.md shows.

**O is held while idle**, as the spec requires. After cool season, O and its relay coil stay energised in Off or idle until the first heat call. Reversing-valve coils are continuous-duty, and the self-heat offset is counted per energised relay (`control.c:99`). This is a design note, not a defect.

### 2.2 Matter and Apple Home

- **ControlSequenceOfOperation 4** (CoolingAndHeating) at `matter_bridge.cpp:330`. The features are Heating, Cooling and AutoMode (`:333-336`).
  - SystemMode values are 0, 1, 3, 4 and 5 (`bridge_map.h:27-31`, matching the spec's SystemModeEnum).
  - In v1.6 the cluster's own SystemMode pre-check only refuses Heat/Cool for heating-only or cooling-only sequences (`ThermostatCluster.cpp:1206-1240`), so CSO 4 accepts every value.
  - The bridge refuses the unsupported ones in PRE_UPDATE (`matter_bridge.cpp:226-237`), which the provider turns into a write failure (`esp_matter_data_model_provider.cpp:365-368`). Nit: the status is FAILURE, not CONSTRAINT_ERROR.
- **Deadband.** MinSetpointDeadBand is int8 in 0.1 °C. The cluster multiplies it by 10 (`ThermostatCluster.cpp:1037`). 16 gives 1.60 °C, and every 3 °F pair is ≥ 1.66 °C (`test_every_thermostat_pair_meets_the_cluster_deadband`).
- **Limits.** All eight are set (`matter_bridge.cpp:357-388`) and re-reported after start (`:620-652`). This matters because the cluster checks writes against both the Abs and the Min/Max limits (`ThermostatCluster.cpp:1094-1113`).
  - The heat max plus the deadband fits under the cool max: 2444 + 160 ≤ 2667. The cool min minus the deadband stays above the heat min: 1778 − 160 ≥ 1556. So `CheckCooling/HeatingSetpointDeadband` never refuses a setpoint the thermostat holds.
  - `test_limits_meet_matter_rules` covers this.
- **LocalTemperature** is created null (`:332`) and reported null on a fault (`:505-507`). The spec allows null, and Home shows no reading.
- **ThermostatRunningState** bits match the spec (`bridge_map.h:34-37`).
- **Fan Control.**
  - FanModeSequence **4 = OffHighAuto**. The bridge reports only High (3) and Auto (5), and both are in the sequence. The Auto feature is added (`:422-423`), which a sequence containing Auto requires.
  - In v1.6 Fan Control is a **code-driven** cluster (`data_model_provider/clusters/fan_control/integration.cpp`). Its writes still reach the bridge (provider runs PRE/POST_UPDATE around `cluster->WriteAttribute`, `provider.cpp:376-383`). The bridge's reports also reach it, because `set_val` sends writable attributes through `WriteAttribute` (`esp_matter_data_model.cpp:1035-1037`).
  - So reporting PercentSetting before FanMode is correct: `ComputeFanModeFromPercent(0)` gives Off (`FanControlCluster.cpp:138-143`).
- **E-heat** is an On/Off Plug-in Unit, which Home shows as an outlet (README says to rename it).
  - Caveat: Home's "all outlets", scenes and Siri can toggle it. Turning it on runs strips plus blower: expensive, not unsafe. Turning it off restores the previous mode (`bridge_map.c:202-228`).
- **Certification.** The device has test VID/PID 0xFFF1/0x8000 and the test DAC; iOS shows "uncertified, Add Anyway" (README:111). The passcode 20202021 is public, so anyone in BLE range can commission it while it is uncommissioned (first boot or after a factory reset). Acceptable for a home build, and documented as test credentials.
- **OTA.** `CONFIG_ENABLE_OTA_REQUESTOR` is not set, so there are no remote updates; that also means no remote firmware attack surface. Updates go over USB only. otadata stays blank, so the device always boots ota_0, the 0x20000 slot.

### 2.3 Updating across versions

- **App-only image at 0x20000** keeps the partition table at 0xC000 and the `nvs` partition, which holds the settings record, the fabric and the OpenThread dataset. The README documents both images (`README.md:47-52`).
- **Non-volatile Matter attributes** come back through `attribute::create` → `set_val_internal(…, false)` with no app callbacks (`esp_matter_data_model.cpp:549`). A stale stored SystemMode or setpoint therefore can't overwrite the settings record. The first `bridge_sync` reports everything (`b_valid` false, `:478`), and the limits and deadband are re-set at boot (`:304-321, 392-403`).
- **v1 to v2 settings** are migrated and saved at the first commit (`settings.c:113-120`). On downgrade the older image falls back to defaults with mode Off; this is not documented (S8).

### 2.4 Toolchain

- ESP-IDF v5.5.5, esp-matter v1.6 and CHIP 93abd8e6 build together and run on hardware (CLAUDE.md status).
- u8g2 is pinned to 2.36.18 by tag (`matter.sh:47-52`).
- **Host tests and firmware.** Nothing persisted depends on the host ABI. `settings_t` is pinned (size 18, every offset) by `_Static_assert` in `settings_logic.c:20-41`, which compiles in both builds. The logic uses fixed-width types throughout, and the native env builds with `-Wconversion -Werror`.

---

## 3. Likely criticisms that do not hold

1. **"The all-off exemption lets O flip inside min-off during normal operation."** It doesn't: see §1.2 (`hvac_logic.c:287-303, 385-400`; chain assert `test_main.c:101-111`). The only callers that request it are `hvac_fault` and `relays_apply(NULL)`.
2. **"The min-run hold can keep the compressor running when it must stop."** It can't: see §1.3. Faults are handled before demand (`:544-546`); Off and e-heat are excluded (`:256`); and the hold reads the driven Y1 after reconcile (`:441`).
3. **"The `matter_sync` self-filter is dead code because `report()` passes `call_attribute_callbacks=false`."** It is load-bearing. In v1.6, `set_val` sends writable attributes through `provider::WriteAttribute`, which runs PRE/POST_UPDATE (`esp_matter_data_model.cpp:1035-1037`; `provider.cpp:365-383`). The thermostat's `EnsureDeadband` side-write (`ThermostatCluster.cpp:478-505`) also goes through `emberAfWriteAttribute` → `set_val_internal(attr, val)` with `call_callbacks` defaulting to **true** (`esp_matter_data_model_priv.h:86`). Without the filter, a heat report moving the pair upward would come back as a cool "write".
4. **"Fan reports never reach Home, because Fan Control is a code-driven cluster with its own state."** They do: writable reports go through `WriteAttribute` → `FanControlCluster::WriteAttribute`. Only PercentCurrent, which is not writable, lands in esp-matter storage instead (S9, harmless).
5. **"Persisted Matter state can override the user's mode at boot."** It can't: the restore runs without callbacks, the settings record is the single source of truth, and the first sync reports every value.
6. **"`settings_update` holding a mutex across a callback can starve control."** It can't: see §1.4. Pure µs callbacks, priority inheritance, NVS writes outside the lock, no nested locks, and a watchdog reset as the backstop.
7. **"A hang leaves the relays energised indefinitely."** The worst case is about 5 s: TWDT panic, the panic handler's RTC WDT, then reset with the pins as inputs and pulled down (§1.6).
8. **"A downgrade could read garbage settings."** The length mismatch is refused by NVS, so the defaults apply with mode Off.
9. **"The setpoint reports could be refused by the cluster's deadband check."** The pre-check compares only against the max cool or min heat (`ThermostatCluster.cpp:381-395`), and the ranges leave room, so reports always pass.

---

## 4. Genuine issues found (most severe first)

| ID | Sev | Issue | Where |
|---|---|---|---|
| S1 | Medium (carried over: A3 ruled CONFIRMED, not applied) | No plausibility window on CRC-valid readings. A sensor returning a valid-CRC −49 °F (or any value) is "valid": heat and aux run continuously, with only the strips' own limit switches as a backstop. A 0–120 °F window would at least catch the gross cases; a stuck-plausible value stays a single-sensor residual and should be documented. | `sensor_logic.c:87-127` |
| S2 | Low (carried over: A5) | `FORBIDDEN_RELAY_PINS` for C6 lacks GPIO4 and GPIO5 (strapping per IDF `gpio/esp32c6.inc:182`), 12/13 (USB-JTAG) and 24–30 (flash). HARDWARE.md:65 repeats the short list. Nothing is wrong today (the relays are on 1, 2, 21, 18), but a re-pin to 4 or 5 would pass the lint. | `check_safety_sources.py:63-66`, `HARDWARE.md:65` |
| S3 | Low (carried over: A4) | A failed `esp_task_wdt_add` is only logged, so the control loop would run unwatched. It can't happen with `TASK_WDT_INIT=y`, but the hard rule depends on it. | `control.c:176-180` |
| S4 | Low (docs, compatibility) | No "compatible / not compatible" statement: O-only, single stage, electric strips, single transformer, C wire required. A B-valve or dual-fuel install would misbehave silently. | README "Safety", HARDWARE.md:3-16 |
| S5 | Low (procedural) | The sim build drives the real relay pins from a simulated room; only a README/CMake warning and "SIM" on the OLED stop it reaching the wall. Any setting saved under sim, including its Auto default, carries into the real build on a `-app.bin` update. Suggest that the sim build refuses to drive relays if an SHT40 answers at 0x44. | `matter/CMakeLists.txt:32-39`, `settings.c:103-108`, `relays.c` |
| S6 | Low (latent) | `bridge_set_limit` writes MinHeat → MaxHeat → MinCool → MaxCool. The cluster cross-checks each against the others (`ThermostatCluster.cpp:1137-1186`), so a future range change that moves both ranges can have MinHeat refused against the old MinCool. `bridge_report_limits` doesn't retry. Today the attributes are created with their final values, so nothing fails. | `matter_bridge.cpp:357-388, 620-652` |
| S7 | Low (tooling, carried over: D3) | `check_size.py` RAM check fails open (`"none"` gives ratio 0.0 and a pass). CLAUDE.md says the flash budget is 85%; `matter.sh` uses 0.90. | `check_size.py:63, 87`; `matter.sh:81` |
| S8 | Low (docs, carried over: E7) | Stale docs and code: CLAUDE.md still describes the S3 bring-up env and PlatformIO S3 build, and says "Not started: the C6 / Thread build". `wifi_register_commands` and the "and Wi-Fi" comment remain in a Thread-only image. That a downgrade resets the settings to defaults is undocumented. | CLAUDE.md; `matter_bridge.cpp:742-745`; `sdkconfig.defaults:57` |
| S9 | Info | PercentCurrent (not writable) is reported into esp-matter storage, but reads come from the code-driven FanControlCluster. The report is a no-op; the cluster computes its own value. | `matter_bridge.cpp:580-582` |
| S10 | Info | Every boot re-arms min-off, but e-heat (G+W) starts at once. A crash loop in e-heat would cycle the strip relays at the loop rate. This is tolerable, since strips have no restart limit, and needs a crash loop to happen at all. | `hvac_logic.c:385-395` |
