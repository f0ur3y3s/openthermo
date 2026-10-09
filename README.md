# openthermo

An open-source smart thermostat for a US heat pump with electric aux strips,
controlled from Apple Home. It runs on a **Seeed XIAO ESP32-C6** as a
**Matter over Thread** device; an Apple HomePod (or Apple TV) is the Thread
border router. It replaces a Braeburn 1220NC.

## Will it work with my system?

**Compatible:** a single-stage air-source heat pump with an **O** reversing
valve (energised in **cool**), electric strips on one W terminal used for
both aux and emergency heat (W1/E jumpered to W2), one 24 VAC transformer (R;
Rh and Rc jumpered), and a C wire. That is the Braeburn 1220NC in its O,
single-stage, electric-aux setting.

**Not compatible:**

- **B** reversing valves (energised in heat, e.g. Rheem/Ruud): heat would cool.
- Two-stage compressors (Y2), or separately staged W2/E.
- Dual fuel (heat pump plus gas furnace).
- A conventional furnace or AC with no heat pump.
- Split Rh/Rc on two transformers.
- No C wire.
- Millivolt or line-voltage systems.

**Good to know:**

- O stays energised while idle, and in Off after cooling.
- There is no outdoor-temperature lockout for the strips.
- There is no hardware delay-on-break; rely on the outdoor unit's own anti-short-cycle delay.
- The device uses Matter test credentials (uncertified).

## Documents

- Wiring, pin map, power and parts: [docs/HARDWARE.md](docs/HARDWARE.md)
- HVAC logic and the safety rules: [docs/CONTROL_SPEC.md](docs/CONTROL_SPEC.md)
- Coding standard (BARR-C plus project rules): [docs/CODING_STANDARD.md](docs/CODING_STANDARD.md)

## Repo layout

| Path | What it holds |
|---|---|
| `components/` | The thermostat: control loop, HVAC logic, relays and their guard, sensor, display, D-pad, settings. |
| `matter/` | The firmware's `idf.py` project: `app_main()` and the Matter bridge. |
| `test/` | Host unit tests for the pure-logic modules. |
| `tools/` | Build, size, safety and log-check scripts. |
| `cad/` | The enclosure, as a Fusion 360 script. |

## Build

The firmware builds in **WSL2** (Debian or Ubuntu), because esp-matter does
not build on native Windows. Flashing is done from Windows.

**One-time setup**, inside WSL:

```sh
bash /mnt/e/esp/openthermo/tools/setup_matter_wsl.sh
```

It installs ESP-IDF v5.5.5 and esp-matter `release/v1.6` under `~/esp`. The
download is several GB and takes a while. Running it again is safe.

**Build**, inside WSL:

```sh
bash /mnt/e/esp/openthermo/tools/matter.sh build        # release: the real thermostat
bash /mnt/e/esp/openthermo/tools/matter.sh build debug  # release plus the serial shell
bash /mnt/e/esp/openthermo/tools/matter.sh build sim    # bench build (debug), see below
```

The release build has no serial shell, to save space; the installed
thermostat needs none. The debug and sim builds add it: `matter esp
factoryreset`, `matter esp attribute get|set ...`, and the bench reset checks
`matter esp reboot` and `matter esp panic`. Their images carry `-debug` or
`-sim` in the name.

- The first build takes a long time, because it compiles connectedhomeip. Later builds are incremental.
- Before building, it checks that only `relays.c` can reach the relay pins.
- Afterwards, it fails the build if the app uses more than 90% of its flash slot or more than 80% of static RAM.

Each build writes two images to `matter/out/`:

| Image | Flash at | Use |
|---|---|---|
| `openthermo-matter-esp32c6[-sim].bin` | `0x0` | First flash, or a clean start. Overwrites the settings area, so the board forgets its pairing and settings. |
| `openthermo-matter-esp32c6[-sim]-app.bin` | `0x20000` | Update. Keeps pairing and settings. |

Other commands: `matter.sh tidy [sim]` (clang-tidy), `matter.sh menuconfig [sim]`, `matter.sh clean [sim]`.

## Flash the XIAO C6 (Windows)

You need esptool. PlatformIO already bundles it
(`~/.platformio/penv/Scripts/python.exe -m esptool`), or install it with
`python -m pip install esptool`.

1. Plug the XIAO in by USB-C, and find its COM port in Device Manager under *Ports (COM & LPT)*. It appears as *USB Serial Device*. The examples use `COM10`.
2. From the repo root, flash.

   **First flash, or start clean:**

   ```sh
   python -m esptool --chip esp32c6 -p COM10 erase-flash
   python -m esptool --chip esp32c6 -p COM10 -b 460800 write-flash 0x0 matter/out/openthermo-matter-esp32c6.bin
   ```

   Erasing first also clears any older firmware with a different partition table.

   **Update, keeping pairing and settings:**

   ```sh
   python -m esptool --chip esp32c6 -p COM10 -b 460800 write-flash 0x20000 matter/out/openthermo-matter-esp32c6-app.bin
   ```

   For the bench build, use the `-sim` images instead.

   **When the bootloader changed** (the build log says so; it holds the
   relay-safety hook), add it to the same command. It sits below the settings,
   so pairing and settings are kept:

   ```sh
   python -m esptool --chip esp32c6 -p COM10 -b 460800 write-flash 0x0 matter/out/openthermo-matter-esp32c6-bootloader.bin 0x20000 matter/out/openthermo-matter-esp32c6-app.bin
   ```
3. esptool resets the board when it finishes. Open the serial console:

   ```sh
   pio device monitor -p COM10 -b 115200
   ```

   If `pio` isn't on your PATH, use `python -m serial.tools.miniterm COM10 115200` instead.

**If esptool can't connect** (*No serial data received*, or the port is missing):

1. Hold the **B** (BOOT) button.
2. Tap **R** (RESET), then release B. The board is now in download mode.
3. Flash again.
4. Press R once more afterwards to run the new firmware.

After a clean start, the board has forgotten the Home it was in. If Apple Home still lists it, remove it there before pairing again.

## Pair with Apple Home

1. An unpaired thermostat starts on its **Pair** page, which shows the QR
   code and the manual pairing code; it is also under Settings › Pair with
   Home. The serial log prints them too:
   - `SetupQRCode: [MT:Y.K9042C00KA0648G00]`;
   - `Manual pairing code: [34970112332]`.

   These are the Matter test credentials (passcode 20202021, discriminator 3840).
2. In the Home app, choose **+ › Add Accessory**. Scan the QR code on the display, or enter the manual code.
3. iOS warns about an **uncertified accessory**: choose **Add Anyway**. The firmware uses Espressif's test attestation certificates, which is expected for a home-built device.
4. The iPhone passes the Thread network credentials to the board over Bluetooth. The board then joins your HomePod's Thread network.

Home shows four accessories:

- **Thermostat:** modes Off, Heat, Cool and Auto; heat and cool setpoints from 60 to 80 °F; the room temperature.
- **Fan:** Auto or On. On runs the blower (G).
- **Outlet:** emergency heat. Rename it "Emergency heat", and keep it out of scenes and "all outlets" commands, which would switch the strips on in place of the heat pump.
- **Contact sensor:** rename it "Thermostat fault" and turn on its notifications. It opens when the thermostat has no valid temperature and has turned everything off.

**Factory reset:** on the D-pad, Settings › Factory reset (the last row),
centre, then Down to Yes and centre again. The prompt starts on No, with No
above Yes; Left, Right or the screen dimming closes it. It unpairs the thermostat
from Home, resets every setting to its default (mode Off) and restarts. Then
remove the accessory from the Home app before pairing again.

On a debug build, `matter esp factoryreset` in the serial console clears the
pairing only, not the thermostat settings.

## Bench build (no SHT40, no HVAC)

`matter.sh build sim` swaps the SHT40 for a simulated room that warms and
cools in response to the outputs. The real timers still apply. The OLED shows
"SIM", and the XIAO's yellow user LED blinks the output state:

| Pattern | Meaning |
|---|---|
| one blip | idle |
| two blips | fan only |
| three blips | waiting out the compressor lockout |
| solid on | heating |
| on with dips | cooling |
| fast blink | aux or emergency heat |
| slow blink | sensor fault |

LEDs on the relay pins show the outputs too. The wiring is in
[docs/HARDWARE.md](docs/HARDWARE.md).

> **Never flash the bench build to the thermostat on the wall.** It
> ignores the real room temperature. As a backstop, if it finds a real SHT40
> on the bus it refuses to drive the relays at all.

`tools/check_relay_log.py` checks a saved serial log from a bench run against the timing rules.

## Host tests (Windows)

The pure-logic modules have host unit tests, run with PlatformIO:

```sh
pio test -e native
```

You need gcc or clang on the PATH. See [test/README](test/README).

## Safety

The firmware enforces:

- a 5 min compressor minimum off, which also applies at boot;
- a 3 min minimum run, which only a fault, Off or emergency heat may cut short;
- reversing-valve changes only after the compressor has been off for 5 min.

The minimum off and the valve rule are enforced twice, in the HVAC logic and
again in an independent relay guard on its own clock. The minimum run is
enforced once, in the HVAC logic, because a guard that could refuse to turn
the compressor off could also block a fault. Every fault drops all outputs at
once and restarts the 5 min lockout, and every reset (crash, watchdog,
restart) drops the relays at once. A reading outside 0–120 °F is treated as
a sensor failure.

**What the house can lose silently:** heat. A sensor fault turns everything
off (the "Thermostat fault" sensor opens, so turn on its notifications). A
wiped settings record, or older firmware flashed over newer, starts in mode
Off with no alert.

Firmware is not a hardware
guarantee, though: fit a 10 k pull-down on each relay input, and rely on the
outdoor unit's own anti-short-cycle delay. See `CLAUDE.md` and
[docs/CONTROL_SPEC.md](docs/CONTROL_SPEC.md).
