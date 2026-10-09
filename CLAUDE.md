# openthermo

This is an open-source smart thermostat on an ESP32. It replaces a Braeburn 1220NC on a US residential **heat pump with electric aux strips** and is controlled from Apple Home.

- **Target:** Seeed XIAO ESP32-C6 running **Matter over Thread**. An Apple HomePod acts as the Thread border router. It is the only target: the ESP32-S3 bring-up board and its builds were removed in Oct 2026.
- `README.md` has the build, flash and pairing steps.

Read these before working:
- `docs/CODING_STANDARD.md` (BARR-C plus project rules; strict and enforced).
- `docs/HARDWARE.md` (wiring, pin map, power, BOM).
- `docs/CONTROL_SPEC.md` (the HVAC logic and safety rules).

## Repo layout

| Path | What it holds |
|---|---|
| `components/<module>/` | All real code, one module per component, public header in `include/`. The strict warning flags apply here. |
| `matter/` | The firmware's `idf.py` project: `main/main.c` (`app_main()` only) and `components/matter_bridge`. |
| `test/` | Host unit tests for the pure-logic modules (`pio test -e native`). |
| `cad/fusion_case_v3.py` | Enclosure as a Fusion 360 API script. It is the ground truth for every mechanical dimension. |
| `docs/reviews/` | Three-agent review (critic, sympathizer, synthesis) of the v3 enclosure. |

## Build

- **Firmware builds with `idf.py` in WSL2** (decided Oct 2026).
  - The `matter/` project uses ESP-IDF v5.5.5 + esp-matter `release/v1.6`, installed by `tools/setup_matter_wsl.sh`.
  - `tools/matter.sh build [debug|sim]` builds it; `tools/matter.sh tidy [...]` runs clang-tidy. The script also fetches U8g2 and qrcodegen at pinned versions.
  - The plain build is the release: no serial shell (`CONFIG_ENABLE_CHIP_SHELL=n`). `debug` adds the shell (`matter/sdkconfig.defaults.debug`); `sim` is a debug build with the simulated room.
  - Flash from Windows with esptool (see `README.md`).
- **PlatformIO only runs the host tests:** `pio test -e native`.
- **Size:** `tools/check_size.py` runs after every build. It fails if the app is over 90% of its smallest app slot, or static RAM is over 80%. The firmware logs its heap low-water mark every 10 min (`control: health`).
- Keep the control logic free of any Matter or HomeKit types, so the same modules run under either stack and under host tests.

## Firmware architecture

| Component | Responsibility |
|---|---|
| `board` | Pin map for the XIAO C6 (verified against Seeed's pinout), plus `board_init()` for the RF switch. Pin map v2 (Oct 8, 2026): relays on D1, D2, D3, D0 (IN1..IN4), I2C on D9/D8, keys UP D10, DOWN D5, LEFT D4, RIGHT D6, OK D7, matching the round-5 controller board. |
| `hvac_logic` | Pure logic with no IDF calls and full host-test coverage. Takes mode, setpoints, temperature and a monotonic time in ms. Returns the desired outputs (Y1, G, O, W). Owns hysteresis, minimum run and off times, aux staging, auto changeover, and the O-change rules. Its header lists how each ambiguous spec rule was read. |
| `relays` | Drives IN1–IN4. All outputs go low as the first act in `app_main`. `relays_guard` (pure, own state) enforces the interlocks a second time, as defence in depth. |
| `i2c_bus` | The one I2C bus that the OLED and the SHT40 share. |
| `control` | A 1 s task on the task watchdog: sensor, then `hvac_logic`, then relays. Publishes a snapshot via `control_get()`. |
| `sensor` | SHT40 over I2C. Stores temperature as `int16_t` tenths of °F or °C (no stored floats, rule 4). Applies a self-heating offset that depends on which relays are energised. Reports a fault after 2 min without a good read. |
| `display`, `pages`, `buttons` | SSD1306 OLED (u8g2) and the 5-way D-pad, ported from the user's NTP desk clock (`E:\esp\deskmate`). These run on the main/UI task only (rule 9). Key handling is pure and tested (`pages_nav`). |
| `settings` | NVS blob with a version and a migration, per rule 10 (v2, Oct 2026: v1 records migrate on boot). Holds mode, setpoints, fan, units, calibration offset, brightness, the dim timeout and the screen-off time (default 10 min; the panel switches off to stop OLED wear, a key wakes it, a sensor fault keeps it on). Every change is one locked `settings_update()`. |
| `matter_bridge` (`matter/components/`) | Matter Thermostat cluster (heat, cool, auto, off; occupied heating and cooling setpoints; local temperature). Fan control cluster for G. Emergency heat is a separate On/Off plug-in unit endpoint (Apple Home has no native e-heat mode); its StartUpOnOff is null so a reboot never cancels e-heat. A "Thermostat fault" contact sensor opens on a sensor fault. Callbacks are ignored until `esp_matter::start()` returns and on the sync task's own reports; limits, deadband, control sequence and StartUpOnOff are set at boot and refused to controllers. Left / right cycle main, mode, fan and settings; Info and Pair with Home are settings rows. The Pair page shows the commissioning QR code (qrcodegen) and manual code; an unpaired thermostat boots onto it. Settings › Factory reset (confirmed, starts on No) resets the settings and unpairs. |

## How the timers are enforced

- **`hvac_logic`:** times the rules and is told what was really driven each cycle. It holds Y1 one extra cycle past minimum run, so the physical run is at least 3 min. Minimum run is enforced here only: a guard that could refuse an *off* could block the fault path.
- **`relays_guard`:** re-checks minimum-off and the O rule on its own clock read in `relays.c`. An all-off request (a fault) is honoured in full and restarts its lockout. It keeps its state twice (bit-inverted); a mismatch means all off and the lockout restarts.
- **Resets:** every reset drops the pins, and both layers re-arm the lockout at boot.
  - On the C6, a panic, a watchdog and `esp_restart()` are CPU-only resets, which keep the GPIO latches. Measured on the bench (Oct 2026): a relay stayed energised about 0.67 s into the reboot.
  - So the relays are dropped three ways: a wrap of the panic handler, a shutdown handler for `esp_restart()` (both `relays_drop_all_now()`), and a 2nd-stage bootloader hook (`matter/bootloader_components/relay_boot_safe`) that runs tens of ms after any reset.
  - `relays_init()` logs a warning if a relay pin was still driven when the app started. `matter esp reboot` and `matter esp panic` on the console are the bench check.
  - Crash-loop breaker (`components/app/boot_guard.c`): after 3 crash resets in a row, Matter stays off and every output is held off for 60 s after boot.
- **Proof:**
  - Host tests: `test_relays_guard` (randomised, with corruption) and `test_safety_chain` (whole path, resets, clock skew, fault bursts; every output off on every fault step; minimum run waived only for a fault, Off or e-heat).
  - `tools/check_safety_sources.py` runs before every build and test, so only `relays.c` can reach the pins.
  - `tools/check_relay_log.py <serial log>` checks a bench run.
- **Not covered by firmware:**
  - pins floating for the tens of ms between a power-on and the bootloader hook: fit 10 k pull-downs;
  - a true hardware guarantee: an outdoor-unit anti-short-cycle delay, or an inline delay-on-break timer on Y;
  - **silent loss of heat**: a sensor fault turns everything off (Home's "Thermostat fault" contact sensor opens; turn on its notifications), and a wiped settings record (NVS erase, which also unpairs) or a firmware downgrade starts in mode Off with no alert at all.

## Hard safety rules (never relax)

- **Outputs:**
  - Heat = Y1 + G.
  - Cool = O + Y1 + G (O is the reversing valve and is energised in COOL).
  - Aux = add W.
  - Emergency heat = G + W, with Y1 always off.
  - Fan = G.
- **Compressor timers:**
  - Minimum off is **5 min**, and the timer also starts at boot.
  - Minimum run is 3 min. Only a sensor fault, mode Off or emergency heat may end a run sooner (a reset does too, by dropping the pins). A change between heat and cool holds the run to 3 min first.
- **Reversing valve:** O may only change while Y1 has been off for at least the minimum-off time. The one exception is a fault (below).
- **Faults:** on a sensor fault, a watchdog reset or a brown-out, all outputs go off, O included, at once. A fault that drops O inside the minimum-off time restarts the 5 min lockout from that moment, exactly as a reset does.
- **Boot:**
  - The relay module is **H-trigger** (high = energised).
  - The relay pins must be driven low as early as possible.
  - No relay pin may be a strapping pin or a UART TX pin.
- **24 VAC side:** C is board GND (half-wave rectifier). Never attach a scope ground to anything but C.

## Status (Oct 2026)

- **Parts:** all ordered (see `docs/HARDWARE.md`). The XIAO C6 boards, mouse switches and spare OLEDs are on hand.
- **Enclosure:**
  - v3 landscape, 138 × 114 × 27 mm.
  - Three-agent reviews done and fixes applied (round 3, wiring/placement/design, Oct 2026: `docs/reviews/v3r3_*.md`).
  - It mounts on the old Braeburn screw line: two screws side by side, measured 80 mm apart, wire hole centred. Left slot horizontal (accepts 72–88 mm), right slot vertical (±3.5 mm to level).
  - Fasteners are M2/M3 button heads threaded straight into PETG; no inserts. Cover rim fit (CLR 0.15) confirmed on a fit-ring print.
  - Centre D-pad key is guided by two 1.75 filament pins (printed pins could snap).
- **Measured (Oct 2026):**
  - fuse holder installed height 15.0 mm, perfboard top to highest point (limit 17.9; model `FUSE_H` still 17.0);
  - XL7015 board 43.8 × 16.1 mm (model 44.0 × 16.0), tallest part 14.56 mm from the PCB bottom, so about 12.96 above its top (limit 13.4: fits with 0.44 to spare; model `XL_H`, measured from the PCB bottom, still 12.0).
- **Measure before printing:**
  - which end of the XL7015 is IN and which is OUT;
  - SHT40 hole position;
  - whether the case covers the old paint outline;
  - D-pad: every key clicks before it bottoms (else set `FL_Z0 = ZF - 1.0`).
- **Before wiring to the HVAC:** confirm the relay NO/COM/NC order and 3.3 V pull-in (`docs/HARDWARE.md`, bring-up step 2).
- **Firmware:**
  - The firmware has the control loop, relays with an independent guard, the SHT40, OLED pages with burn-in creep, the D-pad and NVS settings. The pure logic is host-tested with `pio test -e native`. The self-heat offset per relay (`SENSOR_SELF_HEAT_PER_RELAY_F10`) is 0 until it is measured.
  - **Pin map v2 (Oct 8, 2026):** builds clean and passes `check_safety_sources.py`, but is **not yet bench-validated**. Before any HVAC use: flash the bootloader too (it holds the relay pins), scope all four IN pins (IN4 is now GPIO0) through power-on, reset and flashing, and re-run the relay reset and panic checks in `docs/HARDWARE.md`.
  - **Bench build:** `tools/matter.sh build sim` is the bench build. It runs Matter over Thread with no SHT40: a simulated room responds to the outputs, and the user LED (GPIO15) blinks a pattern per output state (`OPENTHERMO_STATUS_LED`, `status_led.h`). The OLED shows "SIM". Never flash it to the installed unit.
  - **Matter over Thread on the C6 works (Oct 2026):** paired with Apple Home via the HomePod; it runs as a Thread router.
  - **Flashing:** flash the `-app.bin` at 0x20000 to update and keep pairing and settings (plus `-bootloader.bin` at 0x0 when the bootloader changed); the merged `.bin` at 0x0 starts clean.
  - Mode, setpoints (limited to 60–80 °F), fan and e-heat round-trip with Apple Home. The OLED and the status LED work on the bench.
  - Real-hardware testing with the SHT40 and relay module is under way on the bench (Oct 2026).

## How the user works

- Asks for a "triple agent review" by name on hardware designs and BOMs. Run a critic agent and a sympathizer agent in parallel, then a synthesis agent that verifies the disputed points and rules on each.
- Prefers Amazon Prime parts; compatible clones are fine.
- Prints PETG on a Bambu A1 mini and a Snapmaker U1. Wants TPU parts printed as separate inserts.
- Models in Fusion 360, driven through the Fusion MCP. The script deletes and rebuilds the document it runs in, and it refuses to run if the active document holds other work.
