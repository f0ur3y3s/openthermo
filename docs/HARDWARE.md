# Hardware

## HVAC wiring at the wall

The system is a heat pump with electric aux strips, controlled by a 2-heat / 1-cool thermostat.

| Wire | Old terminal | Function |
|---|---|---|
| Red | Rh, jumpered to Rc | 24 VAC hot (one transformer) |
| Blue | C | 24 VAC common, which is board GND |
| Yellow | Y1 | Compressor |
| Green | G | Blower |
| Orange | O | Reversing valve, **energised in COOL** |
| White | W1/E, jumpered to W2 | Aux and emergency strips |

B is unused.

## Power

```
R ──F1 T1A──1N4007──┬─ 470µF 63V ─┬─ 1.5KE51A ─┬─ XL7015 (set 5.00 V) ──┬─ relay module DC+
                    │             │            │                        └─1N5819─ XIAO 5V
C ──────────────────┴─────────────┴────────────┴─ GND (relay DC−, XIAO GND)

R ──F2 T1.6A── relay COM1..COM4 (jumpered)
```

- **Half-wave rectification.** It keeps C = GND, so plugging USB into a PC while the board is on 24 VAC is safe. Check C-to-earth continuity first anyway.
- **Bus voltage.** The DC bus sits at about 33–39 V peak. The XL7015 accepts 5–80 V. Set its output to 5.00 V **before** connecting any load, then lock the trimpot with a dab of nail polish.
- **Relay contacts.** The contacts switch R onto Y1, G, O and W. The SRD relays are rated 10 A.
- **F2 sizing.** About 1.25 A is the contactor *inrush*, which a T fuse rides through; the steady Heat + aux load is typically 0.6–1.2 A. After installation, clamp-meter R in heat, cool, heat + aux and e-heat. Keep T1.6A if the steady current is ≤ 1.1 A, otherwise fit T2A (18 AWG and the contacts allow it).

## Relay module (AEDIKO 4-ch, H/L trigger)

- **Board (measured).** The PCB is 73 × 50 mm, with Ø3.1 mm holes on a 67 × 44.5 mm pattern. Relays stand 15.5 mm above the board, the screw terminals 10 mm.
- **Terminals.** All connections are screw terminals: a 12-way block along one long edge (NO, COM, NC for each relay) and a 6-way block for DC+, DC−, IN1–IN4. There is no pin header to remove. Mount the module with the 12-way block toward the wire window.
- **Jumpers.** Set all four trigger jumpers S1–S4 to **H**. This board has no JD-VCC jumper.
- **Clearance.** The relay tops sit 1–2 mm under the cover. The module must sit flat on its four bosses.
- **Light leaks.** Cover the on-board relay LEDs with Kapton so they don't glow through the top vents.
- **Terminal order is not confirmed.** The CAD assumes NO, COM, NC from the left of each group. Bring-up step 2 checks it before any field wire lands; paint-mark the four NO screws.
- **3.3 V drive.** The IN inputs must pull in from a XIAO GPIO (3.3 V), not just from 5 V. If they don't (see step 2), set S1–S4 to **L** and add one NPN per channel (2N3904, 2N2222 or a 2N7000): collector to IN, emitter to GND, base from the GPIO through 2.2–4.7 k, and keep the 10 k pull-down on the GPIO. GPIO high still means energised and a floating or low GPIO still means off. Never use an inverting pull-up stage in H mode: every relay would turn on at boot.

| Channel | Load |
|---|---|
| IN1 | Y1 |
| IN2 | G |
| IN3 | O |
| IN4 | W |

## Pin map

**Board: Seeed XIAO ESP32-C6.** The assignments live in `components/board/include/board.h`, so re-pinning is a single edit.

| XIAO pin | GPIO | Use |
|---|---|---|
| D1 | 1 | IN1, Y1 |
| D2 | 2 | IN2, G |
| D3 | 21 | IN3, O |
| D10 | 18 | IN4, W |
| D4 | 22 | I2C SDA (SHT40 + OLED) |
| D5 | 23 | I2C SCL |
| D0 | 0 | D-pad up |
| D8 | 19 | D-pad down |
| D9 | 20 | D-pad left |
| D6 | 16 | D-pad right, **through a 1 k series resistor**. This is also U0TXD: the boot ROM drives it as the serial TX line at reset, so holding the key then would short a driven output to ground without the resistor. |
| D7 | 17 | D-pad centre |

- The D-pad switches each pull to GND and use the internal pull-ups.
- Pins a relay must never use, enforced by `tools/check_safety_sources.py`: strapping GPIO 4, 5, 8, 9 and 15; USB-Serial-JTAG GPIO 12 and 13; UART0 TX GPIO 16; SPI flash GPIO 24–30. The RF switch uses GPIO 3 and 14; leave them alone.
- Checked against Seeed's XIAO ESP32-C6 pinout (Oct 2026). GPIO15 is the on-board user LED and a strapping pin; GPIO3 (low = RF switch on) and GPIO14 (low = built-in antenna) are set by `board_init()`.
- The on-board user LED (GPIO15) blinks the output state in the bench (sim) build only.
- **Fit a 10 k pull-down from each IN pin to GND.** After a power-on the pins are floating inputs until the bootloader hook drives them low, tens of ms later.
- **After any other reset** (a crash, a watchdog, a software restart) the C6 keeps its GPIO outputs. The firmware drops the relays in the panic handler, in the restart path and again first thing in the bootloader (Oct 2026 bench check: without these, a relay stayed on about 0.67 s into the reboot).

## Controller board

- **Perfboard** about 74 × 32 mm (x −60..14 in the CAD). Trim the right edge so it stops 0.75 mm short of the divider rib and the sensor grommet.
- **470 µF** lies flat on the board, axis at x = 4 (body x −1..9), so a driver still reaches the (11.5, −49) mounting screw.
- **Pads at the right end** (x 10..14): SHT40, 4 pads at y −42..−36; XL7015, 3 pads at y −34..−29. The cover harness lands at x 2..8, y −28..−24. Keep every pad at least 3 mm from a screw centre.
- **Underside:** only 3 mm to the backplate. Trim every lead and header tail to ≤ 2 mm, and run the 18 AWG R, C and F2 links on the component side.
- **Fuse holders:** the header pins added to the clip legs must be **soldered**, not glued (CA glue doesn't conduct and F2 carries up to about 1 A). Tug-test them and check ≤ 0.05 Ω from clip to pin. Installed height, board top to the highest point, must be ≤ 17.9 mm (the cover has a 1 mm pocket over the fuses).

## Sensor

- MusRock SHT40 module, measured 12.56 × 10.5 mm, 2.85 mm hole near one corner, pins VIN GND SCL SDA on the opposite edge. I2C address 0x44.
- It lies flat, sensor side up, in its own vented chamber at the bottom right of the case, 14 mm off the wall plane. One M2 × 6 goes into a slim standoff, and the board rests on two thin posts (slim on purpose: they carry wall heat into the sensor).
- The wires pass through a TPU grommet in the double-skin divider. Knife-cut a slit from the top of the grommet down to its channel and press the 4 × 26 AWG cable in; the cover squeezes it shut.
- Firmware applies an offset that depends on how many relays are energised; each coil dissipates about 0.35 W.
- **Calibration:** the backplate still pulls the reading 16–28 % of the way toward the wall-surface temperature. Log it against a reference thermometer over a cold night before trusting the offset, especially on an exterior wall.

## UI

- 0.96" SSD1306 I2C OLED at 0x3C. Remove its header and solder wires directly.
- 5-way D-pad made of five lever micro switches, the same type as the NTP desk clock.
- **Cover connectors** (so the cover can come off): one JST-PH 6-pin pair for the D-pad and one JST-PH 4-pin pair for the OLED, inline on pigtails (PH is 2.0 mm pitch, so it doesn't fit the perfboard). The mated pairs lie flat under the D-pad carrier with about 80 mm of cover-side slack; tack them to the backplate with hot glue.
  - D-pad 6-pin: GND, UP, DOWN, LEFT, RIGHT, OK.
  - OLED 4-pin and the optional SHT40 4-pin: **both in the same order, GND, 3V3, SDA, SCL**, so swapping them does no harm.
  - Paint pin 1 on every housing.
- **The relay cable has no connector.** Solder it at the controller and screw it into the module's 6-way block. A 6-pin plug there could mate with the D-pad's, and a key press would then put 5 V on a GPIO or short the 5 V rail.
- **Strain relief:** hot-glue the OLED wires to its PCB next to the pads, and tie or glue the D-pad bundle to the carrier.
- **I2C pull-ups:** check both modules. If the SHT40 module has none, fit 10 k from SDA and SCL to 3V3 on the controller (with the OLED's 4.7 k that gives about 3.2 k).
- **Centre key guides:** two 7.9 mm lengths of 1.75 filament, pushed into the blind holes in the centre key; they slide in the carrier's columns.

## BOM (all ordered from Amazon, Oct 2026)

| Part | Qty used |
|---|---|
| Seeed XIAO ESP32-C6 (3-pack) | 1 |
| AEDIKO 4-ch 5 V relay module, optocoupled (2-pack) | 1 |
| XL7015 DC-DC buck, 5–80 V in (3-pack) | 1 |
| MusRock SHT40 module (2-pack) | 1 |
| 1.5KE51A TVS | 1 |
| uxcell 5×20 PCB fuse holders | 2 |
| BOJACK 5×20 slow-blow fuses: T1A (F1), T1.6A (F2) | 2 |
| Cermant 470 µF 63 V electrolytic, 10×20 | 1 |
| 1N4007 and 1N5819 (from the ALLECIN diode kit) | 1 each |
| 100 nF ceramics (from the BOJACK kit) | several |
| 5.08 mm screw terminals (from the QSYZAIL kit) | R, C plus field wires |
| 0.96" SSD1306 OLED | 1 (on hand) |
| Lever micro switches | 5 (on hand) |
| Perfboard about 74 × 32; M3 × 10 and M2 × 6 button-head screws (threaded straight into PETG, no inserts); #8 screws and anchors | — |
| JST-PH pigtail pairs: one 6-pin (D-pad), one or two 4-pin (OLED, optional SHT40) | — |
| 1 k resistor (D6 line), 10 k pull-downs (IN1–IN4) | 1 + 4 |
| Optional: 10 k I2C pull-ups; 2N3904 / 2N2222 / 2N7000 relay drivers + 2.2–4.7 k base resistors | 2; 4 + 4 |
| 1.75 mm filament (switch retention pins, centre key guides) | — |

## Bring-up order

1. Power stage alone on the bench, from a 24 VAC supply or the real R/C wires:
   - check the bus voltage;
   - set the XL7015 to 5.00 V;
   - check ripple under a 0.5 A load.
2. Relay module, **before any field wire lands**:
   1. Module unpowered: each terminal you plan to use must read **open** to its COM. The third terminal of each group reads **closed** to COM; that one is NC and stays unused.
   2. Power the module from 5 V and drive each IN from a **XIAO GPIO at 3.3 V** (not from 5 V). The relay must pull in solidly, the used terminal must close to COM, and the IN current should be at least 1.5–2 mA. If not, switch to L mode with NPN drivers (see the relay module section).
   3. Paint-mark the four NO screws.
   4. Optional: scope the IN pins through a reset and a flash; they must stay below about 1 V.
3. C6 bench (sim) build: outputs on LEDs, display and D-pad, with the HVAC logic running against a simulated room, paired with Apple Home over Thread. Done (Oct 2026).
4. C6 real build with the SHT40 and the relay module, still driving **LEDs, not the HVAC system**.
5. Wire to the real system with the compressor timer verified. Test fan, then heat, then cool, then aux, then emergency heat.

## Bench checks (C6, before the wall)

Run these on the bench with LEDs (or the relay module) on IN1–IN4 and no HVAC attached. The `matter esp ...` commands need a debug build (`tools/matter.sh build debug`); the serial console is `pio device monitor -p COM10 -b 115200`.

| Check | How | Pass |
|---|---|---|
| Relays drop on a reset | Energise a relay (fan On), then `matter esp reboot`, and again with `matter esp panic`. | The relay drops at once, and the next boot logs `relays: relay pins idle at start-up`, not `still driven`. |
| E-heat survives a reboot | Turn e-heat on in Home, wait 5 s, then power-cycle; repeat with `matter esp reboot` and with an `-app.bin` flash. | Still e-heat on the OLED and in Home; no `from Matter:` line during boot. |
| Sensor fault | Pull the SHT40's SDA lead. | After 2 min: every output off, OLED fault, Home's "Thermostat fault" opens. Reconnect: valid again only after 1 min of good reads. |
| Bench build on a real board | Flash the `-sim` build with the SHT40 connected. | Log: `SHT40 found ... will not drive its relays`; the relays stay off. |
| Fixed attributes | `matter esp attribute set 0x1 0x201 0x19 127` (deadband). | Refused; Home setpoint changes still work. |
| Factory reset | Settings › Factory reset: centre, No, centre; then centre, Yes, centre. | No does nothing. Yes shows "Factory reset", restarts in mode Off, and the Pair page shows the QR code. |
| Timing rules | Run a day with the real build, saving the log; `python tools/check_relay_log.py bench.log`. | `RESULT: PASS`. |
