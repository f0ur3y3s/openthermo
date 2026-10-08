# Firmware review 2, Oct 2026: synthesis (safety and compatibility)

This report rules on the critic's findings (`fw2_2026-10_critic.md`, C1–C14) and on the sympathizer's (`fw2_2026-10_sympathizer.md`, S1–S10 and its defences). It also carries forward the items from `fw_2026-10_synthesis.md` that were confirmed but not yet applied: A3, A4, A5, C-3 and D3. Those are pending, not rejected.

**What was checked, and how**
- Every disputed claim was re-read in the code and traced through the installed sources in WSL Debian:
  - esp-matter `release/v1.6` at c6607128fedc83cb65d6324c14d3e4d7a4d6bd0a;
  - connectedhomeip 93abd8e6891bb578ea63254fb29d099936f345c8;
  - ESP-IDF v5.5.5 (b774170f).
- The resolved configs were read for both builds, `~/build/openthermo-matter-esp32c6{,-sim}/sdkconfig`. Both use the **legacy** data model: `CONFIG_ESP_MATTER_ENABLE_GENERATED_DATA_MODEL` is not set, so `data_model/legacy/*` is what is compiled.
- `python tools/check_safety_sources.py` passes ("relay path OK").
- `pio test -e native` passes: 11 suites, 154 cases.
- No firmware was built and no hardware was run. Every "bench" item below is still to be done.

## Rulings

| ID(s) | Finding | Ruling | Reason (one line) |
|---|---|---|---|
| **C1** | E-heat is cancelled to Heat on every reboot | **CONFIRMED (fix, safety-critical)** | Traced end to end in v1.6 legacy (§1). It fires on every boot where e-heat was on. The persisted OnOff is restored first, which is exactly why the StartUp write is a real change. |
| C1 (other start-up writes) | Could SystemMode, the setpoints or the fan be changed at boot too? | **REJECTED** | The thermostat init callback is empty (`ThermostatCluster.cpp:971`). FanControl is code-driven, with its own state. Non-volatile restores run without callbacks. Only the OnOff StartUp write reaches us. |
| **C2** vs sympathizer §1.6 and defence 7 | A panic, TWDT or `esp_restart` reset keeps the relays driven | **CONFIRMED (fix, safety-critical)**; the defence is **REJECTED** | `esp_restart_noos()` ends in `esp_rom_software_reset_cpu(0)`, a CPU-only reset, and no GPIO reset is done anywhere on the path (§2). "Resets: drop the pins" and CONTROL_SPEC "Any reset: all outputs start low" are false for these resets. |
| C3 / S5 | The sim build drives the real relays | **CONFIRMED (fix)** | `relays.c` has no `OPENTHERMO_SIM_SENSOR` gate. The `-app.bin` images sit side by side, and one wrong path puts a fictional room in charge of the house. |
| C4 / S1, S2, S3, S7 | Prior A3, A4, A5, C-3 and D3 are not in the code | **CONFIRMED (fix)** | `sensor_decode` accepts any CRC-valid value. `control.c:176-180` only logs. `FORBIDDEN_RELAY_PINS` = {8, 9, 15, 16}. `app.c` has no breaker. `check_size.py:63,87` passes at 0 %. |
| C5 | Loss of heat is silent | **PARTIAL (docs now; feature is a user decision)** | Null LocalTemperature is the only signal, and Home does not notify on it. An NVS erase or a downgrade gives Off, unpaired. The mechanism is real; the fix is a new endpoint. |
| C6 | E-heat is an Outlet: Groups, Scenes, Lighting, writable StartUpOnOff | **PARTIAL** | Refusing StartUpOnOff writes is confirmed. Refusing OnTime and OffWaitTime is **rejected**: the OnOff server writes them itself through the same callback path (`on-off-server.cpp` `setOnOffValue`), so refusing them breaks Off and OnWithTimedOff. The outlet exposure is real but low (strips have limit switches; it costs money, it is not dangerous): docs plus a user decision. |
| C7 | `g_restore_mode` race | **CONFIRMED (fix, low)** | Possible only if the sync task blocks in `control_get` between its `settings_get` and its store, and two Home writes land inside that window. Narrow, but a one-line fix. |
| C8 | A flaky sensor gives repeated sub-3-min runs | **PARTIAL (fix)** | Real: one good read clears the fault (`sensor_logic.c:216-229`). But min-off still bounds starts to one per 5 min plus the run, so this is short runs, not short-cycling. Add fault-clear hysteresis. |
| C9 | The logic and guard re-arm on different clocks | **CONFIRMED (fix, low)** | It needs the guard's lag at the stop edge to exceed its lag at the fault cycle. The result is a burst of refused starts, never an unsafe output. |
| C10 / S4 | O-only, held O in Off, no compatibility statement | **CONFIRMED (docs)**. B-valve support is a **user decision**. "Drop O in Off" is **DEFER** | CONTROL_SPEC mandates "hold". Dropping O on Off adds valve cycles and is a spec change, not a fix. |
| C11 / S9 | PercentCurrent | **CONFIRMED (fix, low)** | PercentCurrent is `ATTRIBUTE_FLAG_NONE`, so the report lands in esp-matter storage. Reads are served by the code-driven `FanControlCluster::mPercentCurrent`, which stays 0. |
| C12 | Limits, deadband and StartUpOnOff are writable and never re-asserted | **CONFIRMED (fix, low)**. **New: ControlSequenceOfOperation is writable and non-volatile too** | Legacy flags: Min/Max limits, MinSetpointDeadBand and CSO are `NONVOLATILE | WRITABLE`. CSO = 2 (heating only) would make the cluster refuse Cool. The boot code re-sets the limits and the deadband, but not CSO. |
| C13 | esp-matter tracked by branch | **CONFIRMED (fix)** | `setup_matter_wsl.sh:16,41` clones `release/v1.6` at `--depth 1`. |
| C14 | `check_relay_log.py` false FAIL near 5 min | **CONFIRMED (fix, low)** | Log ticks are taken after `relays_apply` and can be preempted. The guard times on its own `esp_timer` read. |
| S6 | Limit-write ordering | **CONFIRMED (latent, low)** | The cluster cross-checks each limit against the others. Today's values never conflict; a future range change could. |
| S8 | Stale docs and Wi-Fi leftovers | **CONFIRMED (docs)** | `matter_bridge.cpp:742-745`. CLAUDE.md "Not started: the C6 / Thread build" contradicts its own status section. |
| S10 | A crash loop in e-heat cycles the strip relays | **PARTIAL (fold into C-3)** | Tolerable and needs a crash loop to happen at all. The breaker should add a boot hold-off once it trips. |
| Sympathizer defence 5 | "Persisted Matter state can't override the mode at boot" | **REJECTED (in part)** | The restore is callback-free, but the OnOff StartUp rule is a normal write with callbacks (C1). |
| Sympathizer defence 7 | "A hang leaves the relays on for at most about 5 s" | **REJECTED** | The panic handler's RTC WDT is only a backstop. `panic_restart()` → `esp_restart_noos()` does a CPU reset first, and the pins stay driven until `relays_init()`, or indefinitely in a pre-`app_main` loop. |
| Sympathizer defences 1–4, 6, 8, 9 | | **Upheld** | They were not disputed, and were spot-checked. |

## Details on the disputed points

### 1. C1: the e-heat endpoint's start-up write

**The path in v1.6 legacy, step by step:**
1. `on_off_plug_in_unit::add` (`legacy/esp_matter_endpoint.cpp:528-542`) creates OnOff and calls `on_off::feature::lighting::add(on_off_cluster, &config->on_off_lighting)`.
2. The legacy lighting config defaults `start_up_on_off(0)`, which is **Off** and not null (`legacy/esp_matter_feature_impl.h:242`). `bridge_create_eheat` never touches `cfg.on_off_lighting`.
3. `create_start_up_on_off` is `WRITABLE | NONVOLATILE | NULLABLE` (`legacy/esp_matter_attribute.cpp:1564-1569`). `create_on_off` is `NONVOLATILE` (`:1538-1542`).
4. **The persisted values are restored first.** `attribute::create` reads a non-volatile value from NVS into storage with no callbacks (`esp_matter_data_model.cpp:531-546`). OnOff therefore comes back **true** if the unit was in e-heat. The sync task's report of OnOff stored it, because `set_val_internal` persists non-volatile attributes whatever `call_callbacks` says (`:752-766`).
5. `esp_matter::start()` → `chip_init` → `esp_matter_chip_init_task` (on the CHIP task, while the main task waits) → `endpoint::enable_all()` (`esp_matter_core.cpp:229`) → the OnOff init function `emberAfOnOffClusterServerInitCallback` (`legacy/esp_matter_cluster.cpp:1193`) → `initOnOffServer`.
6. `initOnOffServer` (`on-off-server/codegen/on-off-server.cpp:280-321`):
   - Lighting is supported, and both attributes are non-volatile per the esp-matter stub `emberAfIsKnownVolatileAttribute` (`ember_stubs.cpp:655-663`).
   - `getOnOffValueForStartUp` returns false for StartUpOnOff = 0 (`:330-363`; the OTA boot-reason exception does not apply, as there is no OTA).
   - It then calls `setOnOffValue(ep, false→Off::Id, true)`.
7. `setOnOffValue` sees current = true and Off, so `newValue = false`, and calls `Attributes::OnOff::Set` → `emberAfWriteAttribute` (`ember_stubs.cpp:730-780`) → `set_val_internal(attribute, &val)` with **`call_callbacks = true`** by default (`private/esp_matter_data_model_priv.h:86`) → PRE_UPDATE, then **POST_UPDATE** (`esp_matter_data_model.cpp:696-739`).
8. In `bridge_attribute_cb`:
   - `g_h_sync` is still `nullptr`, because the task is created after `start()`, so the echo skip does not apply.
   - `g_eheat_ep` was already assigned (`matter_bridge.cpp:716`).
   - `bridge_edit` sees mode EHEAT, so it does not store a restore mode. `bridge_map_mode_from_eheat(false, EHEAT, g_restore_mode)` returns **Heat**: the default, because `bridge_sync` has never run.
   - `settings_update` marks it dirty; it is saved 3 s later.

**When it fires:** on every boot with OnOff persisted true, which means every boot while in e-heat. That covers a power blip, a brown-out, a TWDT or panic, and an `-app.bin` flash. With OnOff false, `setOnOffValue` returns early ("already set") and nothing happens.

**Consequence:** 5 min after boot, Y1 runs on a compressor the user declared unusable. In a heat-pump-failed winter, W stages only on top of Y1, so the house may get no strip heat until aux staging triggers. This defeats "Emergency heat = G + W, with Y1 always off" in intent: the user's e-heat command is lost.

**Other start-up writes (asked):**
- **Thermostat:** `emberAfThermostatClusterServerInitCallback` is an empty TODO, and the plugin init only registers the AAI. SystemMode and the setpoints are restored callback-free.
- **Fan Control:** code-driven (`data_model_provider/clusters/fan_control/integration.cpp`). Its state is its own, and its init writes nothing through the esp-matter store.
- **Our own boot code:** `bridge_set_limit` and the deadband `set_val` run on the main task before `start()`. They do reach the callback, because these are writable attributes going through `WriteAttribute`, but they hit `bridge_thermostat_write`'s `default:`. They are harmless today, but each sets `g_b_resync` and takes the settings lock.

**Fix (all three; the gate is the load-bearing one):**
1. **A boot gate.** Ignore every callback until `esp_matter::start()` has returned. All cluster init runs synchronously inside `start()` (the main task blocks on `xTaskNotifyWait` until `esp_matter_chip_init_task` finishes). Before that, nothing is a controller's request, and the settings record is the only truth at boot. The first `bridge_sync` already reports every value (`g_reported.b_valid == false`), so whatever start-up wrote into Matter is overwritten within 1 s.
   - On Thread, no controller can reach the node in the microseconds between the end of chip init and the flag being set, because the Thread attach takes seconds. A write dropped there would be reverted by that full first pass anyway.
2. **StartUpOnOff = null.** Set `cfg.on_off_lighting.start_up_on_off = nullable<uint8_t>()`. The attribute is non-volatile, so existing units keep their stored 0. Also set it explicitly at boot, as the deadband is (`attribute::set_val`, before `start()`, inside the gate). This removes the Off-then-On flicker Home would otherwise see after a reboot in e-heat.
3. **Refuse controller writes of StartUpOnOff** in PRE_UPDATE (with C12). Do **not** refuse OnTime or OffWaitTime (see C6).

```cpp
// False until esp_matter::start() returns. Until then esp-matter is running
// the clusters' start-up code (the On/Off StartUpOnOff rule writes OnOff) and
// our own boot-time writes: none of it is a controller's request, and at boot
// the settings record is the only truth. The first bridge_sync reports every
// value, overwriting whatever start-up wrote.
std::atomic<bool> g_b_live{ false };

// in bridge_attribute_cb(), replacing the first test:
    if ((nullptr == p_val) || !g_b_live.load() ||
        ((nullptr != g_h_sync) && (xTaskGetCurrentTaskHandle() == g_h_sync)))
    {
        goto done;
    }

// in matter_bridge_start(), straight after a successful esp_matter::start():
    g_b_live.store(true);
```

The PRE_UPDATE refusals (C12) then sit *after* the gate, so our own boot writes of the limits, the deadband, CSO and StartUpOnOff are never refused.

**Proof (bench, sim build on the C6):**
1. Turn e-heat on from Home.
2. Wait 5 s.
3. Power-cycle. Repeat with a console restart and with an `-app.bin` flash.
4. After each boot, the OLED and Home both show e-heat. The log has no `from Matter:` line during boot. `matter esp attribute get <eheat-ep> 0x6 0x4003` returns null.
5. Repeat with mode Cool, then e-heat on, then reboot, then e-heat off: the result is Heat (the documented fallback after a reboot, B6). That is correct.

### 2. C2: what a reset does to the relay pins on the ESP32-C6

**Which reset each event produces (IDF v5.5.5):**

| Event | Path | Reset level | GPIO latches |
|---|---|---|---|
| Panic, `abort()`, `ESP_ERROR_CHECK`, TWDT (`CONFIG_ESP_TASK_WDT_PANIC=y`), INT WDT, HW stack guard | `panic_handler` → `panic_restart()` → `esp_restart_noos()` (`port/panic_handler.c:310-321`) | **CPU** (`esp_rom_software_reset_cpu(0)`, `system_internal.c:145`); reason `RESET_REASON_CPU0_SW` | **kept** |
| `esp_restart()` (CHIP factory reset, console, our code) | shutdown handlers → `esp_restart_noos()` (`esp_system.c:47-64`) | **CPU** | **kept** |
| Brown-out | ISR → `esp_rom_software_reset_system()` (`esp_hw_support/power_supply/brownout.c:79-81`) | System | reset |
| Bootloader hang (RTC WDT 9 s, `CONFIG_BOOTLOADER_WDT_TIME_MS=9000`) | RTC WDT stage 0 | System | reset |
| Power-on | | Chip | reset (pins are inputs; the 10 k pull-downs hold IN low) |

**The evidence:**
- `soc/esp32c6/include/soc/reset_reasons.h`: "CPU Reset: Reset CPU core only". Core reset (`RESET_REASON_CORE_SW`, via `esp_restart_noos_dig()`) resets the HP digital system. On the C6, IDF uses it only for the ESP32 cache-error case.
- `esp_system_reset_modules_on_exit()` (`system_internal.c:33-93`) resets UART, MSPI, SYSTIMER, GDMA, PWM, ETM and crypto, but **not GPIO or IO_MUX**. Its ETM comment says GPIO state survives a CPU reset.
- `port/soc/esp32c6/clk.c:252-255` deliberately skips peripheral clock gating after the `CPU0_*` reset reasons. IDF's own design preserves peripheral state across CPU resets.
- The 2nd-stage bootloader touches only the console UART and flash pins (`bootloader_console.c`, `bootloader_common.c` for the factory-reset/test GPIO, which is unused here). Nothing touches 1, 2, 18 or 21.

**Why the sympathizer's "RTC WDT resets the chip" does not hold:**
- `esp_panic_handler_enable_rtc_wdt()` (`panic.c:204-212`) arms a **10 s** `RESET_RTC` stage as a hang backstop (`PANIC_REBOOT_DELAY_SECONDS=0` gives `+10 s`). With `PANIC_PRINT_REBOOT`, `panic_restart()` runs first and does a CPU reset.
- `esp_restart_noos()` then reprograms the RTC WDT (1 s, `RESET_SYSTEM`) to guard the boot. The bootloader reprograms it to 9 s, and the app disables it at `init_disable_rtc_wdt` (`startup_funcs.c:180-189`, priority 999).
- So the system reset never happens in a normal panic.

**What the house sees:**
- **(a) Every panic, TWDT or `esp_restart`.**
  - Y1, G, O and W keep their last driven levels through: the panic print (USB-Serial-JTAG console), the ROM, the bootloader (INFO logging plus full SHA validation of an app around 1.5 MB, `SKIP_VALIDATE_*` unset), app start-up (CHIP static constructors), and `app_main` until `relays_init()` (`app.c:82`).
  - This is unmeasured. It is plausibly a few hundred ms to about 1 s.
  - It is harmless in itself, since it extends a legal state, but it contradicts the documented rule.
  - A TWDT on a hung control task already means up to 5 s of stale outputs before the panic. That part is inherent.
- **(b) A deterministic crash after `init_disable_rtc_wdt` but before `app_main`.**
  - Examples: a CHIP static constructor, or heap or esp_system init.
  - It is a CPU-reset loop with no system reset ever (each cycle is under 9 s, and the app keeps disabling the RTC WDT).
  - The last outputs stay energised **indefinitely**. In e-heat that is G + W regardless of temperature; in Cool, Y1 + O + G with no thermostat in control.
  - No trigger is known. The consequence is unbounded, so this is safety-critical, and the fix is cheap.

**Min-off implication.** In loop (b), Y1 is never re-armed, because `relays_init()` never runs: the compressor runs continuously. A loop that reaches `relays_init()` (any crash in `app_main` or later) is safe: every boot drops the pins and re-arms the 5 min lockout.

**Fix: three layers, all write zeros only.**
1. **Bootloader hook (the catch-all, every reset type).** Add a `matter/bootloader_components/relay_boot_safe/` component. Its `bootloader_before_init()` latches the four relay pins low and enables them as outputs. It runs right after the ROM, tens of ms after any reset, before flash init and image validation. It includes `components/board/include/board.h` (header-only; it needs only `sdkconfig.h` and `soc/gpio_num.h`, both in the bootloader build) so the pin map stays single-sourced.
2. **Panic wrap (drops at the instant of the panic, before the print).** Add `-Wl,--wrap=esp_panic_handler` from the relays component's CMake. `__wrap_esp_panic_handler` calls `relays_drop_all_now()` (IRAM, register-level, no driver) and then `__real_esp_panic_handler`. `CONFIG_BT_LE_CONTROLLER_LOG_WRAP_PANIC_HANDLER_ENABLE` (the only other wrapper, in `bt/controller/esp32c6/bt.c:490`) is unset; add an `#error` if it is ever set.
3. **`esp_register_shutdown_handler(relays_drop_all_now)`** in `relays_init()`, for `esp_restart()`.

```c
// relays.c: the only code that writes the relay pins (rule kept).
// Drops all four outputs at once, without the driver or the guard: usable
// from the panic handler and the shutdown path. Dropping is always allowed;
// the next boot re-arms minimum-off in relays_init() and hvac_init().
void IRAM_ATTR relays_drop_all_now(void)
{
    // gpio_num_t values for these pins are 1..21, so the casts are lossless.
    gpio_ll_set_level(&GPIO, (uint32_t)BOARD_PIN_RELAY_Y1, 0U);
    gpio_ll_set_level(&GPIO, (uint32_t)BOARD_PIN_RELAY_W, 0U);
    gpio_ll_set_level(&GPIO, (uint32_t)BOARD_PIN_RELAY_O, 0U);
    gpio_ll_set_level(&GPIO, (uint32_t)BOARD_PIN_RELAY_G, 0U);
}

// Linker wrap (-Wl,--wrap=esp_panic_handler); the names are fixed by ld
// (deviation from the prefix rule, to be listed in CODING_STANDARD).
void __real_esp_panic_handler(void * p_info);
void IRAM_ATTR __wrap_esp_panic_handler(void * p_info)
{
    relays_drop_all_now();
    __real_esp_panic_handler(p_info);
}
```

```c
// matter/bootloader_components/relay_boot_safe/relay_boot_safe.c
// Runs first in the 2nd-stage bootloader on every reset. A CPU-only reset
// (panic, watchdog, esp_restart) keeps the GPIO output latches, so without
// this a relay that was on stays on until relays_init(). Writes zeros only.
void bootloader_hooks_include(void)
{
}

void bootloader_before_init(void)
{
    // gpio_num_t values are non-negative, so the casts are lossless.
    static uint32_t const s_pins[] = {
        (uint32_t)BOARD_PIN_RELAY_Y1, (uint32_t)BOARD_PIN_RELAY_G,
        (uint32_t)BOARD_PIN_RELAY_O, (uint32_t)BOARD_PIN_RELAY_W,
    };
    uint32_t idx = 0U;

    for (idx = 0U; idx < (sizeof(s_pins) / sizeof(s_pins[0])); idx++)
    {
        gpio_ll_set_level(&GPIO, s_pins[idx], 0U); // latch low first
        esp_rom_gpio_pad_select_gpio(s_pins[idx]);
        gpio_ll_output_enable(&GPIO, s_pins[idx]);
    }
}
```

- Its CMake is as in IDF's `examples/custom_bootloader/bootloader_hooks`, with `-u bootloader_hooks_include`.
- Confirm on the bench that the GPIO clock is up in `before_init` after a power-on reset. If it is not, move the body to `bootloader_after_init()`, which is a few ms later and still before image load.
- The 10 k pull-downs are still needed: at a chip reset the pins are inputs until the hook runs.

**Invariants kept:**
- No path ever drives a relay high outside `relays_apply()`.
- Minimum-off is armed at every boot exactly as now (`relays.c:53`, `hvac_logic.c:507`).
- Dropping Y1 early on a reset is the existing "reset waives minimum run" precedence.
- `check_safety_sources.py` must allow pin access in exactly two places: `relays.c`, and the bootloader hook with a "writes 0 only" pattern check.

**Documentation:**
- CLAUDE.md "Resets: drop the pins" and CONTROL_SPEC "Any reset: all outputs start low" are only true once this lands. Until then, add "CPU resets (panic, watchdog, `esp_restart`) keep the pins until `relays_init()`".
- CLAUDE.md is the user's, so its wording needs approval.

**Proof (bench):**
- Add a sim-only console command (`ot-dbg hang`, compiled only with `OPENTHERMO_SIM_SENSOR`) that stops the control task feeding the TWDT. With e-heat on and a scope or logic analyser on IN1–IN4, the pins must fall within 1 ms of the `Task watchdog got triggered` line, and stay low through ROM and bootloader to `relays_init`.
- `ot-dbg restart` does the same for `esp_restart`.
- For the hook alone: temporarily disable the wrap and the shutdown handler, and check that the pins fall at the ROM-to-bootloader hand-off (around 30–60 ms after the reset), not at `relays_init`.
- Record the measured times in HARDWARE.md.

### 3. C3/S5, C5, C6 and C7

**C3 / S5 (CONFIRMED).**
- In `relays.c`, under `#if OPENTHERMO_SIM_SENSOR`:
  - configure the pins as low outputs;
  - compile `relays_write(pin, true)` to a write of 0;
  - keep the guard, the logic and the LED unchanged.
- If the user wants bench relay testing with the sim, make it an explicit second opt-in, `OPENTHERMO_SIM_RELAYS=1`. That build also refuses to drive the relays if an SHT40 ACKs at 0x44 (S5): seeing an SHT40 is evidence the board is in a real installation.
- Give the sim build its own `CONFIG_CHIP_DEVICE_PRODUCT_NAME` ("openthermo SIM") in a sim sdkconfig fragment, and name the sim images `…-SIM-DO-NOT-INSTALL-app.bin`.
- Add a `check_safety_sources.py` rule that the gate exists in `relays.c`.

**C5 (PARTIAL).**
- **Docs now** (README "Safety" and CLAUDE.md residuals):
  - a sensor fault in winter means no heat, and Home shows only a missing temperature, with no alert;
  - an NVS erase (`app.c:30-35`) or a downgrade gives mode Off, and the erase also unpairs.
- **Two user decisions:**
  - **(i)** Add a Contact Sensor / Boolean State endpoint, "Thermostat fault", that Apple Home can notify on. It is open while `!b_valid` or while the crash-loop breaker is tripped.
  - **(ii)** Choose the default mode after lost settings: Off as now, or Heat at 60 °F for freeze protection. Neither touches a hard rule.

**C6 (PARTIAL).**
- **Refuse StartUpOnOff writes:** confirmed (with C12).
- **Refusing OnTime and OffWaitTime: rejected.** `setOnOffValue` itself writes `OffWaitTime::Set(0)`, `OnTime::Set(0)` and `GlobalSceneControl` through `emberAfWriteAttribute`, which runs our PRE_UPDATE on the same CHIP task as a controller write. A refusal would break e-heat Off and OnWithTimedOff, and nothing can tell the two apart.
- **The outlet exposure** ("all outlets", room scenes):
  - "on" swaps the compressor for strips, which costs money but is not dangerous;
  - "off" restores the prior mode (B6).
  - Document: "Name it 'Emergency heat', keep it out of scenes, and do not use 'all outlets' commands."
- **User decision:** keep the On/Off Plug-in Unit, or build the endpoint by hand without the Lighting feature, Groups and Scenes. That removes StartUpOnOff and all of C1's mechanism at the source, but deviates from the device type's conformance, which matters little for an uncertified, test-DAC device. Apple Home may still show it as an outlet or switch.

**C7 (CONFIRMED, low).** Move the store into a no-op `settings_update` edit, so every write of `g_restore_mode` is serialised with the mode:

```cpp
// settings_update() edit run by the sync task: notes the mode e-heat would
// return to, under the same lock as every mode change. Changes nothing.
bool bridge_note_restore(settings_t * p_cfg, void * p_ctx)
{
    (void)p_ctx;
    if ((uint8_t)HVAC_MODE_EHEAT != p_cfg->mode) // 4 fits
    {
        g_restore_mode.store(p_cfg->mode);
    }
    return false;
}
// bridge_sync(): replace lines 482-485 with
    (void)settings_update(bridge_note_restore, nullptr, bridge_now_ms());
```

### 4. C8 and C9: the timers

**C8 (PARTIAL).**
- `sensor_logic_valid` is true for 120 s after any single good read, so an intermittent link produces: start (after the full min-off), fault 120 s later (min-run is waived by a fault), off, and repeat.
- Min-off still holds: at most one start per (run + 5 min), which is under 9 per hour. The outdoor unit's ASC is not challenged.
- What is lost is the minimum run, for oil return.
- **Fix:** fault-clear hysteresis. Once the 2 min fault is declared, the reading becomes valid again only after `SENSOR_RECOVER_MS` (60 s) of unbroken good reads, and any failed read restarts the count. First boot is unaffected: the fault has never been declared, and the 5 min min-off dominates anyway.
- This only lengthens the off state, so no rule is touched.
- **Tests (`test_sensor_logic`):**
  - one good read inside a fault leaves it faulted;
  - 60 s of good reads clears it;
  - a bad read at 59 s restarts the count;
  - boot is valid on the first good read.
- **`test_safety_chain`:** a flaky pattern (1 good read, 130 s bad) yields no Y1 start.

**C9 (CONFIRMED, low).**
- Two different instants are compared: the logic re-arms on its own elapsed test (`hvac_logic.c:449-455`), and the guard on `!b_off_ok` (`relays_guard.c:89-104`).
- The guard's edge is read later than the logic's at the Y1 stop (lag d1), and at the fault (lag d2). If d1 > d2, the guard can see under 5 min while the logic sees 5 min or more. The guard then re-arms and the logic does not, and on recovery the logic asks and is refused every cycle for up to 5 min.
- **Not unsafe**, because the guard is the stricter layer.
- **Fix:** in `hvac_fault`, re-arm whenever it drops O (remove the elapsed test). The logic is then never less strict than the guard after a fault.
- **Cost:** after a fault that drops a long-held O, the next start waits 5 min from the drop (the safe direction).
- **Invariant:** logic min-off ≥ guard min-off at every fault drop.
- **Test (`test_safety_chain`):** with a guard skew that is larger at stop edges than at fault edges, the guard never refuses more than one consecutive logic-requested Y1 start.

### 5. C10 to C14, S6, S10, and the pending items

**C10 (docs).**
- The 1220NC is installer-selectable O/B. This firmware supports O only (energised in cool), and that must be stated (section D).
- **User decision:** a compile-time `OPENTHERMO_REVERSING_B`, which inverts O only at `relays.c`, keeps the logic and guard in "O = cool" terms, and is shown on the info page. A wrong selection turns heat into cool, so it needs a commissioning check: "call heat, feel the supply air within 5 min".
- Holding O in Off is per spec: DEFER.

**C11.** Replace the PercentCurrent report with a call made under `lock::ScopedChipStackLock`:
- `FanControl::FindClusterOnEndpoint(g_fan_ep)` (declared in `fan-control-server/CodegenIntegration.h:29`);
- then `->SetPercentCurrent(status.applied.b_g ? 100 : 0)`.

That reports the real blower, including G during heat or cool calls.

**C12.** Refuse the following in PRE_UPDATE, after the boot gate:
- thermostat 0x0015–0x0018 (Min/Max Heat/Cool limits), 0x0019 (MinSetpointDeadBand) and 0x001B (ControlSequenceOfOperation);
- OnOff 0x4003 (StartUpOnOff).

Put the ID test in pure `bridge_map` (`bool bridge_map_attr_fixed(uint32_t cluster_id, uint32_t attribute_id)`) so it is host-tested. CSO is non-volatile, so set it at boot like the deadband.

**S6.** Make `bridge_set_limit` return its `esp_err_t`, and run the four Min/Max writes in two passes. The second pass catches any refused against an old neighbour. Log a failure after the second pass.

**C13.** In `setup_matter_wsl.sh`:
- after the clone, run `git fetch --depth 1 origin c6607128fedc83cb65d6324c14d3e4d7a4d6bd0a && git checkout FETCH_HEAD`;
- verify that the connectedhomeip submodule HEAD is 93abd8e6891bb578ea63254fb29d099936f345c8 and fail otherwise;
- record both in README.

**C14.**
- Include the guard's own `now_ms` in the relay log line: `relays_apply` stores it, and control prints `t=%llu`.
- `check_relay_log.py` uses `t=` when present, and otherwise allows 50 ms of tolerance on the min-off and min-run comparisons.

**S10.** Fold into C-3. When the breaker trips (3 or more consecutive panic, watchdog or brown-out resets), also hold **every** output off for `BOOT_GUARD_HOLD_MS` (60 s) after boot. That way no relay can chatter at the crash-loop rate. It only adds off time.

**Pending from the last synthesis (unchanged, still CONFIRMED):**
- A3: plausibility window 0..120 °F;
- A4: `esp_task_wdt_add` failure is fatal at start-up;
- A5: forbidden pins 4, 5, 8, 9, 15 (strapping), 12, 13 (USB-JTAG), 16 (U0TXD), 24–30 (flash);
- C-3: crash-loop breaker;
- D3: `check_size.py` fails closed. Also align the budget: CLAUDE.md says 85 %, `matter.sh:72` passes 0.90; pick one.

## Change list (prioritised)

### (A) Safety-critical

| # | Change | Where | Proof |
|---|---|---|---|
| A1 | Matter boot gate (`g_b_live`, set after `esp_matter::start()` returns); StartUpOnOff null in cfg **and** set at boot; refuse StartUpOnOff writes (C1). | `matter_bridge.cpp:207-224, 429-437, 728-733` | Bench: e-heat, then power-cycle, `esp_restart` and `-app.bin` flash → still e-heat; no `from Matter:` at boot; attr 0x6/0x4003 is null. |
| A2 | Relay drop on every reset: bootloader `before_init` hook, `--wrap=esp_panic_handler` → `relays_drop_all_now()`, and a shutdown handler (C2). Correct the reset wording in CLAUDE.md and CONTROL_SPEC (user approval for CLAUDE.md). | new `matter/bootloader_components/relay_boot_safe/`; `relays.c`, `relays/CMakeLists.txt`; lint | Bench: induced TWDT and `esp_restart` with e-heat on; pins low within 1 ms of the panic and through boot. Hook-only run: low at the bootloader hand-off. Record the times. |
| A3 | Sensor plausibility window 0..120 °F (prior A3, C4/S1). | `sensor_logic.c` `sensor_decode`, `sensor.h` | `test_sensor_logic`: −49 °F, 266 °F and the 0.0/120.0 edges; 2 min of implausible reads gives a fault. |

### (B) Safety

| # | Change | Where | Proof |
|---|---|---|---|
| B1 | Watchdog subscription fatal: `esp_task_wdt_add(h_task)` in `control_start()`, returning its error (prior A4). | `control.c:176-180` | Review. |
| B2 | Crash-loop breaker (prior C-3) plus a 60 s all-off boot hold-off once tripped (S10). Pure `boot_guard` with an `RTC_NOINIT` counter. | `components/app` | Host test of the pure counter: reason sequences give count, `net_allowed` and `hold_ms`. |
| B3 | Fault-clear hysteresis, 60 s of unbroken good reads (C8). | `sensor_logic.c`, `sensor.h` | `test_sensor_logic` (4 cases); chain test: flaky pattern gives no start. |
| B4 | `hvac_fault` re-arms on every O drop (C9). | `hvac_logic.c:449-455` | Chain test with asymmetric skew: no more than 1 consecutive refused start. |
| B5 | Sim build never drives the relays; `OPENTHERMO_SIM_RELAYS` opt-in refuses if an SHT40 is present; sim product name and image names (C3/S5). | `relays.c`, `matter/CMakeLists.txt`, sim sdkconfig, `matter.sh` | Lint rule for the gate. Bench: relay module LEDs dark on the sim build with heat called. |
| B6 | `g_restore_mode` stored inside a no-op `settings_update` edit (C7). | `matter_bridge.cpp:482-485` | Review. |
| B7 | Document loss-of-heat residuals: fault in winter, NVS erase, downgrade (C5). | README "Safety", CLAUDE.md (approval) | Review. |

### (C) Compatibility (code)

| # | Change | Where | Proof |
|---|---|---|---|
| C-1 | Refuse writes to the limits, MinSetpointDeadBand, CSO and StartUpOnOff in PRE_UPDATE via pure `bridge_map_attr_fixed`; set CSO at boot (C12). | `bridge_map.c/.h`, `matter_bridge.cpp` | `test_bridge_map`: every fixed ID is refused, and SystemMode and the setpoints are not. Bench: chip-tool write of MinSetpointDeadBand=127 fails, and Home setpoints still work. |
| C-2 | Two-pass limit writes with checked returns (S6). | `matter_bridge.cpp:307-321, 357-388` | Review. |
| C-3 | PercentCurrent via `FanControlCluster::SetPercentCurrent` from `applied.b_g` (C11). | `matter_bridge.cpp:575-589` | Bench: `attribute get <fan> 0x202 0x3` is 100 while heating. |
| C-4 | Pin esp-matter and connectedhomeip commits (C13). | `tools/setup_matter_wsl.sh`, README | Fresh setup prints and checks both hashes. |
| C-5 | Drop `wifi_register_commands` and the "and Wi-Fi" text (S8). | `matter_bridge.cpp:742-745` | Build. |

### (D) Compatibility (docs): the installation statement for the README

> **Compatible:** a single-stage air-source heat pump with an **O** reversing valve (energised in **cool**), with electric strips on one W terminal used as both aux and emergency heat (W1/E jumpered), one 24 VAC transformer (R; Rh/Rc jumpered), and a C wire. This is the Braeburn 1220NC in its O / single-stage / electric-aux setting.
>
> **Not compatible:**
> - **B** reversing valves (energised in heat; e.g. Rheem/Ruud): heat would cool.
> - Two-stage compressors (Y2), or separately staged W2/E.
> - Dual-fuel (heat pump plus gas furnace).
> - Conventional furnace or AC with no heat pump.
> - Split Rh/Rc on two transformers.
> - No C wire.
> - Millivolt or line-voltage systems.
>
> **Behaviour to know:**
> - O stays energised while idle and in Off after cooling.
> - There is no outdoor-temperature aux lockout.
> - There is no hardware delay-on-break (rely on the outdoor unit's anti-short-cycle).
> - A sensor fault turns everything off, and Apple Home shows only a missing temperature.
> - The device uses Matter test credentials (uncertified).

### (E) Tests and tooling

| # | Change | Where |
|---|---|---|
| E1 | Forbidden relay pins 4, 5, 8, 9, 12, 13, 15, 16 and 24–30 (prior A5). Pin access allowed only in `relays.c` and the bootloader hook, with the hook checked to write 0 only. Rule that the `OPENTHERMO_SIM_SENSOR` gate exists in `relays.c`. | `tools/check_safety_sources.py`, `docs/HARDWARE.md:65` |
| E2 | `check_size.py` exits 1 on `"none"` RAM (prior D3); one flash budget in CLAUDE.md and `matter.sh`. | `tools/check_size.py`, `tools/matter.sh:72` |
| E3 | Log the guard's `now_ms`; the script uses it, else ±50 ms (C14). | `relays.c`, `control.c:82`, `tools/check_relay_log.py` |
| E4 | Sim-only `ot-dbg hang` / `ot-dbg restart` console commands for the A2 bench check. | `components/control` (sim only) |
| E5 | Bench checklist in HARDWARE.md: e-heat survives a reboot (A1); relay drop time on TWDT and restart (A2); sim relays dark (B5); fixed-attribute writes refused (C-1). | `docs/HARDWARE.md` |

## Needs the user's decision

1. **CLAUDE.md wording** (hard rules and "How the timers are enforced"): the reset statement (A2) and the residuals (B7).
2. **E-heat endpoint type:** keep the Plug-in Unit (with A1 and the docs) or a hand-built On/Off endpoint without Lighting, Groups and Scenes (C6).
3. **A fault endpoint** Apple Home can alert on (C5-i), and the **default mode** after lost settings (C5-ii).
4. **B-valve support** via `OPENTHERMO_REVERSING_B` (C10).
5. **Sim relay opt-in** for bench relay testing (B5).
6. **Flash budget:** 85 % or 90 % (E2).
