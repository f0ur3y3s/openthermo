# Firmware review, Oct 2026: synthesis

This report rules on the critic's findings (`fw_2026-10_critic.md`, C1–C12) and the sympathizer's (`fw_2026-10_sympathizer.md`, S1–S9 and its defences).

**What was checked, and how**
- Every disputed claim was re-read in the code.
- Matter behaviour was checked in the installed sources in WSL:
  - esp-matter `release/v1.6`, c6607128, 2026-09-17;
  - connectedhomeip 93abd8e6, 2026-08-07;
  - ESP-IDF v5.5.5.
- `python tools/check_safety_sources.py` passes ("relay path OK").
- `pio test -e native` passes: 11 suites, 140 cases.
- A scratch probe (not in the repo) reproduced C1, C2, C3 and C8. It compiled the real `hvac_logic.c`, `relays_guard.c` and `bridge_map.c` with gcc and wired them as `control.c` does.

## Rulings

| ID(s) | Finding | Ruling | Reason (one line) |
|---|---|---|---|
| C1 / S1 | A sensor fault leaves O energised for up to 5 min | **CONFIRMED (fix)** | Reproduced: O stayed on for 300 s of the fault. The guard should honour an all-off request in full and re-arm min-off, as a reset does. |
| C2 (vs. sympathizer defence C3) | Minimum run is waived on Heat↔Cool | **CONFIRMED (fix)** | Reproduced: a 1000 ms run at the guard's pins. The spec allows only fault or mode-off. Keep the e-heat waiver and document it as a rule precedence. |
| C3 / S2 | End setpoints are reported outside the Matter limits | **CONFIRMED (fix)** | 1556/2444/1778/2667 against 1557/2443/1779/2666. In v1.6, `report()` of a writable attribute runs the thermostat cluster's limit check and fails, and `g_reported` is still updated. |
| **N1 (new)** | `attribute::report()` on a *writable* attribute re-enters `bridge_attribute_cb` | **CONFIRMED (fix)** | v1.6 sends writable attributes through `provider::WriteAttribute`, which runs the app's PRE/POST_UPDATE whatever `call_callbacks` says. Both reviews assumed it did not. |
| C4 / S3 | Settings read-modify-write race | **CONFIRMED (fix)** | UI and CHIP are both priority 1, with no lock across get/set. N1 makes `matter_sync` (priority 4) a third writer. |
| C5 | A boot loop keeps the compressor locked out | **PARTIAL (fix, lower priority)** | The mechanism is real: every boot re-arms 5 min and Matter starts after control. No reproducible trigger is known. |
| C6 | E-heat off always selects Heat | **CONFIRMED (fix, low)** | `bridge_map.c:190-193`, by design but surprising from Cool or Off. |
| C7 | Matter attributes stay out of step after no-op writes | **CONFIRMED (fix)** | `bridge_sync` compares against `g_reported`, not against the attribute. FanMode Off while Auto stays "Off" in Home. |
| C8 | The 3 °F deadband (166–167) is narrower than MinSetpointDeadBand 1.7 °C (170) | **PARTIAL (fix differently)** | The gap is real (probe: 166..167). The stated consequence is not: the legacy thermostat cluster never runs `EnsureDeadband`. Fix: advertise 1.6 °C, not a 3.1 °F deadband. |
| C9 | A failed settings save is never retried | **CONFIRMED (fix, low)** | `settings.c:178` clears dirty before the write. |
| C10 / S5 | Strapping list incomplete; pin lint can be bypassed | **CONFIRMED (fix)**; the bypass part is **PARTIAL** | ESP-IDF v5.5.5 `gpio/esp32c6.inc:182` lists GPIO4, 5, 8, 9 and 15 as strapping pins; 12/13 are USB-JTAG and 24–30 are flash. The lint stays a lint; widen its patterns. |
| C11 | Tests do not prove what test/README claims | **CONFIRMED (fix)**; the clock-skew sub-point is **REJECTED** | No fault→all-off check, faults last 1 step, and every mode change waives min-run. The guard always reads its clock *after* the logic in firmware, so "never earlier" is the real case. |
| C12 / S9 | Stale docs and comments | **CONFIRMED (docs only)** | Each item was spot-checked and is present. |
| S4 (also C12) | `check_size.py` RAM check fails open | **CONFIRMED (fix)** | `static_ram()` returns `(0, 0, "none")`, so the ratio is 0.0 and the check passes. |
| C12 (relay log) | `check_relay_log.py` 32-bit ms wrap | **CONFIRMED (fix, low)** | ESP log timestamps are `uint32_t` ms. |
| S6 | The right key on U0TXD (GPIO16) can short a driven pin | **CONFIRMED (hardware, low)** | The console is USB-JTAG, but the ROM still drives U0TXD at reset. Fit a 1 k series resistor. |
| S7 | No plausibility window on CRC-valid readings | **CONFIRMED (fix)** | Use a window of 0–120 °F, not 32–110 °F: a genuinely cold house must still be able to call for heat. |
| S8 | A failed watchdog subscription is only logged | **CONFIRMED (fix, low)** | `control.c:176-180`. |
| Defence C3 (sympathizer) | "The waiver is the same principle as mode-off" | **REJECTED** | The spec names exactly two exceptions, and Off and fault are both "stop now" commands; a direction change is not. |
| Defence C8 (sympathizer) / critic's "sound" list | "`report()` does not re-enter the callbacks" | **REJECTED** | See N1. |

## Details on the disputed points

### 1. C1 / S1: O on a sensor fault

**What the code does.**
1. `hvac_fault()` requests `0 0 0 0` (`hvac_logic.c:441`).
2. The guard treats O 1→0 as an O change (`relays_guard.c:94-98`).
3. While Y1 is on, `b_off_ok` is false, so the guard holds O = 1.

The probe ran 700 s of cooling and then a 400 s fault: O stayed energised for 300 s.

The same hole also falsifies two documented contracts:
- `relays.h:32` says "NULL drives everything off" (`relays_apply(NULL)` builds an all-zero request, and the guard holds O);
- `relays_guard.h:8` says "Turning anything off is always allowed".

**Which rule takes precedence.** "Sensor fault → all outputs off" wins. Four reasons:
1. **The system already drops O at any instant.** A reset, a brown-out or a watchdog drops O through the hardware, whatever the timers say. The guard's own corruption resync (`relays_guard.c:80-87`) drops O and re-arms min-off. The installation must tolerate an O drop at Y1 stop anyway.
2. **The O rule protects the valve from switching under load.** De-energising O at the moment the compressor stops is what a conventional thermostat does on System Off. It is the "less energy" direction.
3. **Two contracts already promise it:** the guard's own "off is always allowed", and the NULL contract in `relays.h`.
4. **The alternative breaks a hard rule.** Holding O in `hvac_fault` and amending the docs would mean rewriting "all outputs off" in CLAUDE.md.

**Fix: narrow the exemption.** Do *not* let any O drop through. Exempt only a request that is **all-off**, because that is exactly the fault, NULL and reset case. When that exemption is used, restart the min-off timer at that instant, as a reset does.

**Invariants kept (guard):**
- **(I1)** Y1 starts only after Y1 has been off ≥ `MIN_OFF`. An exempt O drop counts as a Y1 off edge, so Y1 also cannot start within `MIN_OFF` of the valve moving.
- **(I2)** O never changes while Y1 stays on.
- **(I3)** O rises (0→1) only after Y1 has been off ≥ `MIN_OFF`. Because of the re-arm, that is also ≥ `MIN_OFF` after any exempt drop.
- **(I4)** O falls only under I3's condition, or on an all-off request, which re-arms.
- **(I5)** W/Y1/O and G-forcing are unchanged.

**What does not change.** Normal operation never sends all-off while O = 1: Off and idle *hold* O (CONTROL_SPEC table), so they request `{G?, O=1}`. The exemption is therefore reached only from `hvac_fault`, a NULL request, or `hvac_step(NULL…)`. A chattering all-off request cannot cycle the valve: each exempt drop re-arms, and O cannot rise for 5 min after it.

```c
// relays_guard.c, inside relays_guard_step(), replacing lines 94-98
    b_all_off = (!p_req->b_y1 && !p_req->b_g && !p_req->b_o && !p_req->b_w);

    if ((out.b_o != p_guard->applied.b_o) && !b_off_ok)
    {
        if (b_all_off)
        {
            // An all-off request (sensor fault, NULL) is honoured in full,
            // as a reset would be: O drops with everything else and the
            // minimum-off time restarts now, so neither Y1 nor O may change
            // again for RELAYS_GUARD_MIN_OFF_MS.
            off_ms = now_ms;
        }
        else
        {
            out.b_o  = p_guard->applied.b_o;
            out.b_y1 = false;
        }
    }
```

`bool b_all_off = false;` is declared at the top, initialised. The later `if (p_guard->applied.b_y1 && !out.b_y1) off_ms = now_ms;` is unaffected.

**Mirror it in `hvac_logic`.** This keeps the two layers in agreement, with no WARN line and no refused restart.
- In `hvac_fault()`, before the `memset`: if `p_state->out.b_o` is set and Y1 has not been off for `HVAC_MIN_OFF_MS`, then:
  - set `y1_edge_prev_ms = y1_edge_ms`;
  - set `y1_edge_ms = now_ms`.
- `b_start_pending` stays false.

**Tests that prove it at the pins:**
- `test_relays_guard`:
  1. `test_all_off_drops_o_and_rearms`. Cool runs 10 min, then all-off. Expect `0 0 0 0` and a passed request. Then `O=1` and `Y1=1` are refused until `drop + MIN_OFF`, and allowed at `drop + MIN_OFF`.
  2. The same with Y1 off for 2 min before the drop. Y1 must still wait a full `MIN_OFF` from the drop.
  3. `test_partial_off_still_holds_o`. Request `{Y1 0, G 1, O 0, W 0}` while cooling. O is held, so the exemption is narrow.
  4. Extend `test_turning_off_is_always_allowed` to all four outputs.
  5. In the fuzz checker:
     - change the O clause to *either* `!prev.b_y1 && now - y1_off >= MIN_OFF` *or* (all-off output *and* all-off request);
     - on such a drop, set `y1_off = now`, so the Y1-start assertion then proves I1.
- `test_safety_chain`:
  - assert all four driven outputs are 0 on **every** step with `!b_temp_valid`;
  - accept an O change that is an all-off drop, and set `y1_off_ms = now` on it;
  - replace single-step faults with bursts of 1–400 steps (a 2 min loss needs 120).
- `tools/check_relay_log.py`:
  - FAIL if a `FAULT` line shows any output on;
  - allow an O 1→0 on an all-off `FAULT` line, and treat it as a re-arm (`y1_off_ms = t`).

### 2. C2: minimum run on a direction change

**The code.** `hvac_min_run_holds` (`hvac_logic.c:248-254`) requires `hvac_mode_permits(mode, call)`, and it is consulted only when `want == NONE` (`:527`). On Heat→Cool:
- `want` becomes COOL;
- the call changes;
- `hvac_drive_compressor` drops Y1 to "wait for the valve".

The probe started a run and flipped to Cool 1 s later: the guard drove a **1000 ms** run.

**The rules.**
- CONTROL_SPEC:22 says "3 min, unless a fault or mode-off overrides it".
- CLAUDE.md says "Minimum run is 3 min" with no exception.
- `hvac_logic.h:23-25` widens this on its own authority, so the code violates the spec as written.

**On the sympathizer's defence.** Its bound is right: a mode toggle can still start the compressor at most once per 5 min. But a 1 s run every 5 min is exactly the short-cycle the minimum run exists to prevent.

**Ruling.** Enforce minimum run on direction changes, and waive it only for:
- **a fault**, which is already handled before demand;
- **mode Off**, which the spec allows;
- **mode e-heat**.

The e-heat case is a genuine conflict between two hard rules: "minimum run 3 min" and "e-heat: Y1 always off". Choosing e-heat is the user declaring the compressor unusable, which is the same "stop now" intent as Off. The existing chain test already asserts no Y1 in e-heat. Document it as the precedence.

**What stays open.** Toggling Off/Heat from Matter can still produce short runs, because the spec allows Off to stop a run. Min-off still bounds the start rate.

```c
// True while a running heat or cool call must continue for minimum run.
// Only Off, emergency heat and a fault (handled before this) end it early.
static bool hvac_min_run_holds(hvac_state_t const * p_state,
                               hvac_input_t const * p_in)
{
    return (p_state->out.b_y1 &&
            ((HVAC_CALL_HEAT == p_state->call) ||
             (HVAC_CALL_COOL == p_state->call)) &&
            (HVAC_MODE_OFF != p_in->mode) && (HVAC_MODE_EHEAT != p_in->mode) &&
            (hvac_elapsed(p_in->now_ms, p_state->y1_edge_ms) <
             HVAC_MIN_RUN_HOLD_MS));
}
```

**Code changes:**
- In `hvac_step`, hold on *any* change, not only on NONE: `if ((want != p_state->call) && hvac_min_run_holds(p_state, &in)) { want = p_state->call; }`.
- In `hvac_drive`, `case HVAC_CALL_HEAT`: after `hvac_drive_heat`, if `!hvac_mode_permits(p_in->mode, p_state->call)`, set `p_state->b_aux = false` and `out.b_w = false`. The hold-over runs the heat pump only; strips are never staged in Cool. Dropping W is always allowed.

**Invariants kept:**
- min-off is untouched;
- O still changes only after the held run ends and a full min-off passes (`hvac_drive_compressor` is unchanged);
- the run at the pins is ≥ 3 min (with the existing 1 s margin) for every end except fault, Off, e-heat and reset.

**Tests:**
- `test_hvac_logic`:
  - "Heat→Cool at 10 s into a run keeps Y1 until 181 s, then waits 300 s, then O and Y1 together";
  - "Cool→Heat" (the mirror);
  - "→Off stops at once";
  - "→e-heat stops at once";
  - "W is dropped during a hold-over".
- `test_safety_chain`:
  - set `b_run_waived` only when the mode *actually changes* to Off or e-heat, or on a fault or reset;
  - all other mode and setpoint changes must now meet `HVAC_MIN_RUN_MS` on the guard's clock.
- `check_relay_log.py`: a short run that ends on a line whose `call` is `heat` or `cool` is a FAIL (the logic now holds the call); `none` or `e-heat` stays a WARN.

**Docs.** README.md:159: "enforced twice" applies to min-off and the O rule. Minimum run is single-layer by design, because a guard that refused an off could block the fault path.

### 3. C3 / S2: the setpoint limits

**Arithmetic.** It was confirmed by the probe.
- `f10_to_c100` gives 600→1556, 760→2444, 640→1778 and 800→2667.
- The limits are each nudged one hundredth inward, to 1557, 2443, 1779 and 2666.

**What v1.6 does with the report.** The path is `attribute::report` → `update_or_report` → `set_val(attr)` (`esp_matter_data_model.cpp:1050`). The setpoints are `ATTRIBUTE_FLAG_WRITABLE`, so the call goes to `set_val_via_write_attribute` (`:1036`), then to `provider::WriteAttribute`. That function:
- runs our PRE_UPDATE;
- has the Thermostat AAI decline (it handles only RemoteSensing and Presets);
- calls `ClusterPreAttributeChanged`, which is `MatterThermostatClusterServerPreAttributeChangedCallback`.

That last check returns `InvalidValue` for `requested < MinHeatSetpointLimit` or `> MaxHeatSetpointLimit` (`ThermostatCluster.cpp`, `OccupiedHeatingSetpoint` case). The report then fails with `ESP_FAIL`, not the `ESP_ERR_INVALID_ARG` the critic cited, though the effect is the same. `set_val_internal`'s bounds from `add_bounds_cb` would also refuse it.

`bridge_sync` then sets `g_reported = now` unconditionally (`matter_bridge.cpp:449`), so the value is never retried.

**Fix: exact limits, neither inward nor outward.** `lim.x = bridge_map_f10_to_c100(HVAC_x_SP_y_F10)`, with no ±1.
- Reports use the same monotonic function on values sanitised into the same ranges. Every report therefore lies inside the limits, and the end values sit exactly on them.
- The Matter gap rule still holds: 1556 + 170 ≤ 1778 and 2444 + 170 ≤ 2667.

**The question asked: can a value inside the limits snap outside hvac_logic's range?**
- In °F mode: no. The single-rounding snap is monotonic, and the exact ends map to 60, 76, 64 and 80. The probe swept every value and found 0 outside.
- In °C mode: **yes, even today.** The probe found 39 values. For example, 1556–1574 snap to 15.5 °C = 599, and 2425–2444 to 761.
- These are saved only because `matter_bridge.cpp:96/103` calls `hvac_setpoints_apply`, and `settings_set` sanitises again. The invariant holds for *snap + apply*, not for the snap alone.
- The `bridge_map.h:69-71` comment ("rounded inward, so a value inside it always converts back inside") is therefore wrong, and the nudge buys nothing.

**Further changes:**
- Move the composite into the pure layer: `void bridge_map_setpoint_write(int16_t c100, bool b_celsius, bool b_heat, int16_t * p_heat_f10, int16_t * p_cool_f10)`. It snaps, then calls `hvac_setpoints_apply`, and the bridge calls it.
- Clamping reports instead was rejected: once the limits are exact it is unneeded, and a clamp would hide a real mismatch.
- Update `g_reported` per field, only when the report returned `ESP_OK`. Log a failure once per value change, not every second.

**Tests (`test_bridge_map`):**
- For every f10 in each range, `f10_to_c100(f10)` lies inside the corresponding limits.
- For every c100 in each limit range, in both units, `bridge_map_setpoint_write` yields `hvac_setpoints_valid(heat, cool)`.
- `test_limits_stay_inside_the_thermostat_ranges` becomes an equality: each limit equals the converted end.

### 4. N1 (new): reports re-enter the attribute callback

**The path.** The same path as in §3 applies to every writable attribute the sync task reports: SystemMode, both setpoints, FanMode and PercentSetting. All are `ATTRIBUTE_FLAG_WRITABLE` in `legacy/esp_matter_attribute.cpp`.
- `provider::WriteAttribute` calls `execute_callback(PRE_UPDATE…)` and then `execute_post_update()` (`esp_matter_data_model_provider.cpp:364-433`), whatever `call_callbacks` says.
- `bridge_attribute_cb` therefore runs **on the `matter_sync` task**, inside the CHIP stack lock. It reads the settings *now* and applies the value that `bridge_sync` read *earlier*.

**Consequences:**
1. **A stale write-back.**
   1. The UI changes a field twice in quick succession: one step is seen by `bridge_sync`'s `settings_get`, the next lands before the report acquires the stack lock.
   2. The POST_UPDATE echo then writes the older value back, and the D-pad step is lost.
   3. Key auto-repeat makes this plausible. The same applies to the e-heat endpoint.
2. **In °C display mode the echo rewrites setpoints.** A whole-°F setpoint is snapped to the half-°C grid (690 → 2056 → 2050 → 689) the first time it is reported.
3. Settings gain a third writer (see C4).

The lock is re-entrant (`chip_stack_lock` checks `IsChipStackLockedByCurrentThread`), so there is no deadlock.

**Fix.** Keep the task handle from `xTaskCreate` (`g_h_sync`). At the top of `bridge_attribute_cb`, if `xTaskGetCurrentTaskHandle() == g_h_sync`, skip all settings handling: it is our own report echoing back.
- The boot-time `bridge_set_limit` calls also echo, from the main task, but they are harmless: the callback's `default:` branch handles them, and nothing changes.

**Test.** This is not host-testable; it is a bench check.
1. With the sim build, hold Up on the D-pad for 5 s.
2. Every step must survive. The serial log must show no `from Matter:` line that was not caused by a Home action.

### 5. C4 / S3: the settings race

**Confirmed.**
- `app_ui.c:78-83` and `matter_bridge.cpp:159-200` each do get→modify→set with no lock across the sequence.
- The CHIP task priority is 1 (`CONFIG_CHIP_TASK_PRIORITY` default, `config/esp32/components/chip/Kconfig:368-371`). That is the same as the main/UI task.

**Fix: one critical section per edit.**

```c
// settings.h
typedef bool (*settings_edit_fn_t)(settings_t * p_cfg, void * p_ctx);

/**
 * Applies p_fn to the current record under the settings lock, then
 * sanitises it and schedules the save if anything changed. p_fn must be
 * short and must not block, log or take any other lock.
 * @return true if the record changed
 */
bool settings_update(settings_edit_fn_t p_fn, void * p_ctx, uint64_t now_ms);
```

```c
// settings.c
bool settings_update(settings_edit_fn_t p_fn, void * p_ctx, uint64_t now_ms)
{
    bool       b_changed = false;
    settings_t cfg       = { 0 };

    if ((NULL == p_fn) || (NULL == g_h_lock))
    {
        goto done;
    }

    (void)xSemaphoreTake(g_h_lock, portMAX_DELAY);
    cfg = g_cfg;
    if (p_fn(&cfg, p_ctx))
    {
        (void)settings_logic_sanitise(&cfg);
        b_changed = (0 != memcmp(&cfg, &g_cfg, sizeof(cfg)));
        if (b_changed)
        {
            g_cfg        = cfg;
            g_b_dirty    = true;
            g_changed_ms = now_ms;
        }
    }
    (void)xSemaphoreGive(g_h_lock);

done:
    return b_changed;
}
```

**Callers:**
- The UI passes a callback that wraps `pages_nav_key` with `{&nav, key, event}` as context. `pages_nav` is pure, so it is safe under the lock.
- The bridge passes `bridge_thermostat_write`, `bridge_fan_write` and the e-heat mapping, with the `ESP_LOGI`s moved out and the change logged after the call returns.

**Lock order.** The bridge already holds the CHIP lock when it takes the settings lock. No task takes the settings lock and then the CHIP lock, so the order is safe.

**Afterwards:**
- Make `settings_set` `static`, or remove it.
- Add a `check_safety_sources.py` rule: `settings_set(` may appear only in `settings.c`. That makes the fix hold by construction, since a timing race is not unit-testable on the host.

### 6. C8: the deadband mismatch

**Confirmed numerically.** For every 3 °F pair, `f10_to_c100(h+30) − f10_to_c100(h)` is 166 or 167, which is below 170.

**The consequence the critic named does not happen in v1.6 (legacy data model):**
- `EnsureDeadband` is called only from `MatterThermostatClusterServerAttributeChangedCallback` (`ThermostatCluster.cpp:947`).
- The legacy thermostat cluster registers only `INIT | PRE_ATTRIBUTE_CHANGED` (`legacy/esp_matter_cluster.cpp:1369-1372`).
- The pre-change check (`CheckCoolingSetpointDeadband` and `CheckHeatingSetpointDeadband`) only tests whether the *other* setpoint *could* be moved inside its limit, so our reports are not rejected.
- The generated data model *does* register `ATTRIBUTE_CHANGED`. If that model is ever enabled, the cluster would re-push the other setpoint by 0.03 °C.

**What is still wrong.** The reported state breaches the cluster invariant `cool − heat ≥ MinSetpointDeadBand`.

**The critic's fix (`HVAC_AUTO_DEADBAND_F10 = 31`) is rejected.** `hvac_setpoints_apply` would then produce x.1 °F setpoints, off the whole-degree grid.

**Fix:**
- Set `BRIDGE_DEADBAND_C10 16`. It is in range 0..127 (`thermostat-cluster.xml:451`, pre-check `:1189-1195`), and every pair the thermostat can hold, 166 or more, then conforms.
- A Home pair 1.6–1.66 °C apart is still widened to 3 °F by `hvac_setpoints_apply`, and the server may legally move the other setpoint.
- `MinSetpointDeadBand` is `NONVOLATILE`, so set it explicitly at boot as the limits are (`bridge_set_limit`-style). Otherwise a stored 17 survives the update.

**Tests:**
- Replace `test_deadband_covers_the_thermostats`, which encodes the wrong direction, with "for every heat f10 on the °F and °C grids, `f10_to_c100(h + HVAC_AUTO_DEADBAND_F10) − f10_to_c100(h) ≥ BRIDGE_DEADBAND_C10 × 10`".
- Keep `test_limits_meet_matter_rules`.

### 7. The rest

**C5, boot loop (PARTIAL).**
- `relays_init` and `hvac_init` re-arm 5 min on every boot (correct). `p_net_start` runs after `control_start`.
- Any reproducible abort within 5 min therefore means no heat at all: W stages only on top of Y1, so in Heat mode nothing heats.
- No trigger is known. It is still worth a cheap breaker, because the failure is silent in winter.
- A breaker never shortens a lockout, so no rule is touched.

**C6 (CONFIRMED, low).** Change the pure function to `bridge_map_mode_from_eheat(bool b_on, uint8_t current_mode, uint8_t mode_before)`.
- `mode_before` is the last non-e-heat mode that `bridge_sync` observed. It is held in RAM, so it also covers e-heat chosen on the D-pad.
- If `mode_before` is unknown or is e-heat, fall back to Heat (for example, after a reboot while in e-heat).
- No NVS change.

**C7 (CONFIRMED).**
- In POST_UPDATE for a Home write (after N1, our own echoes are skipped), compute the value the thermostat will report for that attribute from the resulting settings.
- If it differs from `p_val`, set a bit in a `std::atomic<uint32_t> g_force_report`.
- `bridge_sync` reports any flagged field and clears the bit once the report succeeds.
- Example: 2010 snaps to 680, which reports as 2000, so the attribute is re-asserted within 1 s. FanMode Off maps to Auto (5), which is re-asserted.

**C9 (CONFIRMED, low).**
- On a failed `settings_store`, take the lock. If `!g_b_dirty`, set `g_b_dirty = true` and `g_changed_ms = now_ms + SETTINGS_RETRY_DELAY_MS - SETTINGS_COMMIT_DELAY_MS`, with a 60 s retry. If a newer change already re-dirtied the record, leave it.
- This is host-testable only if the NVS call is behind a seam. Otherwise it is a review-only item.

**C10 / S5 (CONFIRMED pins, PARTIAL lint).**
- ESP-IDF v5.5.5 `docs/en/api-reference/peripherals/gpio/esp32c6.inc:182`: "GPIO4, GPIO5, GPIO8, GPIO9, and GPIO15 are strapping pins". The same note puts USB-JTAG on GPIO12/13 and SPI flash on GPIO24–30.
- U0TXD = 16 and U0RXD = 17 (`soc/esp32c6/include/soc/uart_pins.h`).
- Current relay pins 1, 2, 21 and 18 are clean.

**C11 (CONFIRMED, except skew).** In the firmware, `relays.c:66` reads the clock after `hvac_step`, so a guard clock *earlier* than the logic's is not a real case. The chain test's `now + [0, 50)` skew is right.

**S6 (CONFIRMED, hardware, low).**
- On reset the ROM prints on UART0 even when the IDF console is USB-JTAG, so GPIO16 is a push-pull high until `buttons_init()`.
- Holding the right key shorts it to GND for well under 1 s. That is out of spec, but brief.
- Fit 1 k in series with the D6 key (the internal pull-up is about 45 k, so a press still reads low). Correct the `board.h:28-30` comment.

**S7 (CONFIRMED).**
- In `sensor_decode` (or `sensor_poll`), treat a raw reading outside **0..120 °F** (`SENSOR_PLAUSIBLE_MIN_F10 0`, `MAX_F10 1200`) as a failed read. The 2 min fault rule then applies.
- A 32 °F floor would fault, and so turn off the heat in, a house that has really got that cold. That is the one situation where heat is needed most. Below 0 °F, or above 120 °F indoors, means a broken sensor (or a fire, where all-off is right).
- Also make `sensor_logic_valid` fail closed on `now < last_good` (`sensor_logic.c:222`), for consistency with both timer layers. It is unreachable with `esp_timer`.

**S8 (CONFIRMED, low).**
- Subscribe from `control_start()`: `esp_task_wdt_add(h_task)`, using the handle from `xTaskCreate`, and return its error.
- `app.c` already wraps `control_start()` in `ESP_ERROR_CHECK`, so a failure resets (D3-compliant) instead of running the loop unwatched.

## Change list (prioritised)

### (A) Safety and hard-rule conformance

| # | Change | Where | Proof |
|---|---|---|---|
| A1 | The guard honours an all-off request in full: O drops, min-off re-arms at that instant (invariants I1–I4, §1). Mirror the re-arm in `hvac_fault`. | `relays_guard.c:94-98`, `relays_guard.h` contract text, `hvac_logic.c:433-445` | `test_relays_guard`: all-off drop and re-arm, partial-off holds O, fuzz O-clause update. `test_safety_chain`: all four outputs 0 on every fault step, fault bursts, re-arm on the exempt drop. |
| A2 | Minimum run holds across direction changes; waived only for a fault, Off and e-heat. W is dropped during a hold-over. | `hvac_logic.c:248-254, 527-530, 366-368`; `hvac_logic.h:23-25` | `test_hvac_logic`: 5 cases (§2). `test_safety_chain`: waiver only on a real change to Off or e-heat, a fault or a reset. |
| A3 | Sensor plausibility window: 0..120 °F raw, else a failed read. | `sensor_logic.c` (`sensor_decode`) and `sensor.h` constants; `:222` fails closed | `test_sensor_logic`: −49 °F, 266 °F, 0.0 °F and 120.0 °F edges; 2 min of implausible reads gives a fault. |
| A4 | Watchdog subscription failure is fatal at start-up. | `control.c:176-180` moves to `control_start()` | Review (IDF call). |
| A5 | Strapping, USB and flash pins added to the forbidden relay pins: 4, 5 (strapping), 12, 13 (USB-JTAG), 24–30 (flash). | `tools/check_safety_sources.py:56-59`, `docs/HARDWARE.md:65`, `board.h:27` | Temporarily set a relay pin to `GPIO_NUM_4`: the tool must FAIL. |

### (B) Matter correctness

| # | Change | Where | Proof |
|---|---|---|---|
| B1 | Skip settings handling in `bridge_attribute_cb` when it runs on the `matter_sync` task (N1). Keep the task handle. | `matter_bridge.cpp:142-205, 585-589` | Bench: hold Up for 5 s on the sim build; no step lost, no unprompted `from Matter:` line. |
| B2 | Exact limits (no ±1). Move the write path into `bridge_map_setpoint_write` (snap + `hvac_setpoints_apply`). | `bridge_map.c:92-107`, `bridge_map.h:68-73`, `matter_bridge.cpp:91-104` | `test_bridge_map`: every range value inside the limits; every limit value in both units gives `hvac_setpoints_valid`; limits equal the converted ends. |
| B3 | `g_reported` is updated per field, only on `ESP_OK`. A failure is logged once per value. | `matter_bridge.cpp:361-450` | Bench: set heat to 60 °F on the D-pad; Home shows 60 within 1 s. |
| B4 | Re-assert after no-op or snapped writes via `g_force_report` (C7). | `matter_bridge.cpp` POST_UPDATE path and `bridge_sync` | Bench: write FanMode Off; Home returns to Auto within 1 s. Write 68.2 °F; Home settles on 68. |
| B5 | `BRIDGE_DEADBAND_C10 16`, also set at boot like the limits. | `bridge_map.h:49-51`, `bridge_create_thermostat` | `test_bridge_map`: new deadband test (§6). |
| B6 | E-heat off restores the last non-e-heat mode (RAM); fallback Heat. | `bridge_map.c:181-200`, `bridge_map.h:117-121`, `bridge_sync` | `test_bridge_map`: Cool→e-heat→off gives Cool; Off→e-heat→off gives Off; unknown gives Heat. |

### (C) Robustness

| # | Change | Where | Proof |
|---|---|---|---|
| C-1 | `settings_update(fn, ctx, now)`: one critical section per edit. `settings_set` made static. | `settings.c/.h`, `app_ui.c:76-85`, `matter_bridge.cpp:159-200` | New `check_safety_sources.py` rule: `settings_set(` only in `settings.c`. |
| C-2 | Retry a failed save after 60 s, without clobbering a newer change. | `settings.c:161-197` | Review, or a host test behind an NVS seam. |
| C-3 | Crash-loop breaker (details below). | `app.c:77-108`, plus a pure `boot_guard` helper in `components/app` | Host test of the pure counter: sequences of reasons → count → net allowed. |
| C-4 | 1 k series resistor on the D6 (U0TXD) key. | `docs/HARDWARE.md`, `board.h:28-30` comment | Hardware. |

C-3 details:
- Keep an `RTC_NOINIT` counter with a magic word, counting consecutive panic, watchdog and brown-out resets.
- When the count reaches 3, skip `p_net_start` and show "NET OFF: crash loop" on the OLED.
- Clear the counter after 15 min of uptime.
- The breaker never shortens a lockout.

### (D) Tests and tooling

| # | Change | Where |
|---|---|---|
| D1 | The test changes in A1, A2, A3 and B2/B5/B6 above. Update the test/README coverage claims to match. | `test/` |
| D2 | `check_relay_log.py` changes (listed below). | `tools/check_relay_log.py` |
| D3 | `check_size.py` exits 1 when `static_ram` returns `"none"`. | `tools/check_size.py:52-63, 87` |
| D4 | `check_safety_sources.py` lint changes (listed below). | `tools/check_safety_sources.py` |

D2, `check_relay_log.py`:
- A `FAULT` line with any output on is a FAIL.
- An O drop on an all-off `FAULT` line is allowed and re-arms.
- A short run ending with call `heat` or `cool` is a FAIL.
- Unwrap 32-bit timestamps (if `t < prev_t` with no boot line, add 2³²).

D4, `check_safety_sources.py`:
- Flag `gpio_config`, `gpio_set_direction`, `gpio_reset_pin`, `gpio_ll_`, `gpio_hal_`, `REG_WRITE`, `WRITE_PERI_REG` and `GPIO_OUT_W1T` outside an allow-list (`relays.c`, `buttons.c`, `board.c`, `status_led.c`, `i2c_bus.c`).
- Add the `settings_set` rule from C-1.
- Reword the docstring: "a lint, not a proof".

### (E) Docs

All of these are worded to match A1, A2 and B5. The CLAUDE.md hard-rule text is the user's, so get the user's approval for E1 before editing it.

| # | Change |
|---|---|
| E1 | **CLAUDE.md hard rules** (needs user approval): add the two precedences to the existing rules. |
| E2 | **CONTROL_SPEC.md:** the three precedence lines; a "1.6 °C advertised deadband" note under Interfaces; remove "commission QR" from :47 until it exists. |
| E3 | **README.md:157-160:** "Minimum off and the valve rule are enforced twice (logic and guard); minimum run is enforced by the logic only, by design, because a guard that could refuse an off could block the fault path. A fault drops every output." |
| E4 | **Headers:** `relays_guard.h:8-14` gains the all-off exemption and its re-arm. `relays.h:32` becomes true. `hvac_logic.h:23-25` becomes "waived on Off, e-heat and a fault only". `bridge_map.h:69-71` describes the exact limits. |
| E5 | **CODING_STANDARD.md:** fix rule 2 (`net_bring_up_stack`), rule 9 (`app_events`, also `display.h:7`), D1 (test_weather/cJSON), D3 ("The clock…"), and D5 (`settings_defaults.h` / recovery AP; drop it or mark it unused). |
| E6 | **CLAUDE.md:42** (°F only), **:44** (version, no migration yet), **:45** (the QR is planned, not shown). |
| E7 | **Wi-Fi leftovers and comments:** `matter_bridge.cpp:574-577` drops `wifi_register_commands` and the "and Wi-Fi" wording. Update `sdkconfig.defaults:57`. `settings.c:100-105`: a sim build that saved any setting carries it into the real build on an `-app.bin` update, so flash the merged `.bin` or set Off first. |
| E8 | **HARDWARE.md:** the full C6 forbidden-pin list (A5) and the D6 series resistor (C-4). |

E1 wording:
- "Minimum run is 3 min; only a sensor fault, Off or emergency heat may end a run sooner."
- "On a fault, all outputs go off at once, O included; the min-off timer then restarts, as at a reset."
