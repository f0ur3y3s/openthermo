> **Superseded.** This was the brief for the round-4 board (screw holes, USB-C through the left wall). The board now in the case is round 5 (no mounting holes, cradle, USB-C through the bottom wall); see docs/HARDWARE.md and docs/reviews/board_r5_synthesis.md and case_r6_synthesis.md.

# Controller board: parts the case model still needs

> **Done (Oct 8, 2026), then redone for the real board.** The board on hand is a 30 × 70 perfboard with only 10 × 24
> holes and no mounting holes, so the layout was redone on that grid (docs/HARDWARE.md, "Controller board"). Everything
> stays on the one board; it sits in a cradle on the backplate and pins on the cover hold it down. The XIAO's antenna
> end has 4 mm to the fuse holders' plastic and about 6 mm to their metal clips. The 100 nF at the XL7015 output moved
> onto the XL7015's own terminals, and the optional NPN drivers still don't fit.

This is for whoever next edits `fusion_case_v3.py`. The firmware and the
bring-up work (Oct 2026) settled every part on the controller perfboard.
The model's reference bodies, the `r = [...]` list in `build()`, cover only
the large ones. Add the parts below as reference bodies, find room for them on
the board, and check them against the cover and backplate like every other
body. All units are mm, in the script's frame.

## What the model already has (leave as is)

| Reference body | Where |
|---|---|
| Controller perfboard (`CTL`) | x −60..14, y −52..−20; top face at z = `CTL_TOP` (7.6) |
| Screw holes (`CTL_HOLES`) | (−57, −49), (11.5, −49), (−57, −23), (11.5, −23) |
| XIAO ESP32-C6, female headers, shield and USB-C | x −61.5..−39, y −45..−27.5; USB-C through the left wall |
| Fuse holders F1, F2 (17 tall with cap) | x −36..−9.2, y −51.5..−30.5 |
| 470 µF 63 V, lying | axis at x = 4, y −50.5..−29.5, Ø10 |
| R-C screw terminal | x −10..0, y −29..−21 |
| Pad strip (SHT40 4, XL7015 3) | x 10..14, y −42..−29 |
| XL7015, JST-PH pairs, relay module, SHT40, OLED, switches | off the controller board |

## Parts to add

All of these go on the **top** of the perfboard: there is only 3 mm under it
(docs/HARDWARE.md). Perfboard pitch is 2.54. Heights are above the board's
top face. Sizes are the parts' maximum datasheet bodies plus room for the leads.

| Part | Qty | Envelope (L × W × H) | Mounting | Must sit next to |
|---|---|---|---|---|
| 1.5KE51A TVS (DO-201AD) | 1 | 15.2 × 5.5 × 5.5 lying (body Ø5.3 × 9.5, leads bent at 15.24 pitch) | lying flat | across the DC bus, beside the 470 µF |
| 1N4007 (DO-41) | 1 | 10.2 × 2.8 × 2.8 lying | lying flat | between F1 and the 470 µF (R → F1 → 1N4007 → bus) |
| 1N5819 (DO-41) | 1 | 10.2 × 2.8 × 2.8 lying | lying flat | between the XL7015 output pads and the XIAO's 5V pin |
| 100 nF ceramic (radial disc) | 3 | 5 × 3 × 7 standing | standing | one at the XL7015 output pads, one each at the XIAO's 5V and 3V3 |
| 10 kΩ pull-down, 1/4 W | 4 | 10.2 × 2.6 × 2.6 lying | lying flat | the XIAO pins D0, D1, D2 and D3 (pin map v2), each to GND. **Safety-relevant:** they hold the relay inputs low for the tens of ms after power-on before the firmware drives them. Keep them short, at the XIAO end of the relay cable. |
| 1 kΩ series resistor, 1/4 W | 1 | 10.2 × 2.6 × 2.6 lying | lying flat | in series between XIAO D6 and the D-pad's RIGHT wire. D6 is the chip's serial TX at reset, and this keeps a held key from shorting it. |
| Relay cable pads | 6 | 15.3 × 2.6, plus about 4 of wire bend | solder pads | near the XIAO's D0..D3 side (pin map v2). The wires are DC+, DC−, IN1–IN4, soldered with no connector (a plug could mate with the D-pad's). Allow a strain-relief tie or glue spot. |

**Reserve room for these too.** They may or may not be fitted:

| Part | Qty | Envelope | When |
|---|---|---|---|
| 10 kΩ I2C pull-ups, 1/4 W | 2 | 10.2 × 2.6 × 2.6 lying | only if the SHT40 board has no pull-ups (SDA, SCL to 3V3) |
| NPN driver (2N3904 / 2N2222, TO-92) | 4 | 5.2 × 4.2 × 7.0 standing | only if the relays will not pull in from 3.3 V (bring-up step 2) |
| Base resistor 2.2–4.7 kΩ, 1/4 W | 4 | 10.2 × 2.6 × 2.6 lying | with the NPNs, one each |

The NPN set is the bulkiest maybe. If it cannot fit on the perfboard, say so,
and it can move to a small daughter strip at the relay module's input terminal.

## Rules for placing them

- **Screws:** every pad and lead stays at least 3 mm from a `CTL_HOLES` centre, and leaves a driver path to each screw (the 470 µF is already placed for this).
- **Height:** everything clears the cover's inner face (`ZF` = 25) and stays under the fuse pocket's assumption (fuses ≤ 17.9). Every part above is under 8 tall, so the cover only matters near the fuses.
- **Underside:** no part, lead or header tail under the board beyond 2 mm (trim), so the 3 mm gap holds.
- **Free space, as a starting point:** check these yourself before using them.
  - The band above the fuses, y −30..−21, x −38..−11 (left of the R-C terminal), suits the pull-downs, the 1 kΩ and the relay pads.
  - The strip between the fuses and the 470 µF, x −9..−1, y −52..−30, suits the 1N4007 and the TVS lying along y.

## Clearances the case must also keep

1. **XIAO antenna.** The XIAO ESP32-C6's ceramic antenna is at the end of the board **opposite the USB-C port**; check on the part. That end is at x ≈ −39, about 3 mm from the fuse holders' metal clips (x −36) and next to the 24 VAC wiring. Metal and live wiring that close can detune it and weaken Thread. If you can, move the fuses or the XIAO to leave 5–10 mm around the antenna end with no metal and no 24 VAC wires over it, and leave plastic only (no screws or inserts) in front of it.
2. **XIAO B (boot) and R (reset) buttons.** They sit near the USB-C end on the top face; check on the part. They are only needed to force download mode when flashing fails. Reaching them with the cover off is enough; no opening is needed.
3. **USB-C.** It stays reachable through the left wall (the existing cut). Firmware updates are USB-only: there are no over-the-air updates, and bootloader updates need USB too.
4. **XL7015 trimpot.** It is set once to 5.00 V before any load is connected, then sealed. Keep it reachable with a small screwdriver with the cover off.
5. **Fuses.** F1 and F2 must be replaceable with the cover off, without lifting the perfboard.

## When done

- Add each part as a named reference body in `build()`'s `r` list, as the existing ones are (`tbox(...)` / `tcyl(...)`), so a clash shows in Fusion.
- Update docs/HARDWARE.md, "Controller board", with where each part went.
- Say if anything did not fit, or if the antenna end could not be cleared.
