# v3 landscape enclosure: sympathizer review

Source checked: `/home/claude/thermo/fusion_case_v3.py` (line refs below). All mm. Z=0 is the wall.

## 0. A screenshot discrepancy to clear up first
The front screenshot shows a raised rounded block with its own window at upper centre-left. Nothing in v3 makes that shape: the cover is one shelled box (L199-211) plus internal joins. `build()` deletes only occurrences (L193-194), not root-level bodies. So this is almost certainly a stale body from an earlier run, or a v2 leftover sitting in the root component. It is not a v3 design feature. Delete it and re-screenshot before the critic spends words on it. Everything else in the image matches the code: OLED window upper right, D-pad below it, two bottom vent groups, and the four right-side chamber intakes.

## 1. What the landscape layout gets right

**1.1 Depth from 34 to 27 (−20.6%) comes from putting the stacks side by side.**
- In v2 (96×126×34, v2 L22) the UI stack had to sit in front of the electronics.
- In v3 the two tallest stacks share the plan instead:
  - Relay stack: PCB z 6–7.6, relays to 6+1.6+15.5 = **23.1** (L361), Dupont to **24.1** (L344).
  - UI stack: carrier z 13.5 to the face at 25 (L54-56).
- Depth is now set by max(relay, UI) rather than their sum. That is the right way to get a thin part.
- The spec text says "relays top out around z 25". That treats the SRD's 19 mm *length* as height. The SRD-05VDC body is about 15.5 tall, so the model's 23.1 (**1.9 clear** of ZF 25) is the correct number. Verify with calipers, but this is not a depth problem.

**1.2 The XL7015 and XIAO fit.**
- XL7015 top = 7.6+12 = **19.6**, which leaves **5.4** for the trimmer under ZF 25 (L349).
- XIAO USB shield sits at z 17.1–20.3, centre 18.7. The left-wall slot is z 15–22.5, centre **18.75** (L240, L347). The y centres match exactly at −36.25.
- The slot is 13×7.5, which takes a standard USB-C overmold (about 12.4×6.5).

**1.3 The wiring is a star around the wall penetration.** The wire window (x ±17, y −19..−8, L285) sits in a 13 mm channel between the top of the CTL board (y −20) and the bottom edge of the relay module (y −7). Every wall-wire endpoint is within about 25 mm of it:
- the relay screw terminals directly above (y −7..1, L362);
- F1 to the left (y −18..−10);
- F2 to the right (y −16.5..−8.5);
- the R/C terminal just below at (−10..0, −30..−22).

This gives short 24 VAC runs and no high-current wiring crossing the UI column.

**1.4 The sensor chamber is upstream of everything.**
- Chamber location: bottom right (x 16.5..67, y −55..−32.5).
- It is fed by its own bottom vents (x 21..49, L236) and the right-wall intakes (L238-239).
- It exhausts through a transfer port (L231) up the UI column to its own top vents (x 22..57, L234).
- Relay coils are at y ≥ 3, so the nearest coil (K4 corner, about (7.75, 3)) is about **36.6 mm** from the chamber corner.
- Heat rises away from the chamber, not into it.
- The wall-cavity draft enters at the wire window, which is in the relay bay and outside the chamber. In practice this draft is usually the biggest source of thermostat error, and this layout already isolates it.
- The foam pocket (L286-288) seals it at the source.

**1.5 The divider is already a labyrinth, not a butt joint.** The cover divider (z 10.4–25.3, 1.6 thick, L214-215) drops into a 2.0 groove in the 3.2 backplate rib (L265-268). Air leaking around the joint has to turn twice.

**1.6 The UI column gives clear screw access.**
- The gang holes at (41, ±41.65) (L60, L289-290) sit where the backplate carries nothing tall. The carrier, OLED and keys all ride on the cover.
- With only the backplate up, a driver reaches both screws.
- The bottom slot is horizontal ±4 on an 83.3 pitch, which gives about **±2.75°** of levelling.

**1.7 The cover is a self-contained UI module.** OLED, keys and switch carrier all hang from cover posts (L225-226, L246-247). The backplate holds everything on the 24 VAC side. This split is right for install and for repair.

**1.8 Retention is belt and braces.** There are snap ridges (r0.6 at x ±66.75, which gives 0.35 interference into the cover groove, L232-233 and L263-264) plus a hidden bottom M3 into a heat-set boss (L248, L277, L284).

**1.9 Printability**
- Bed fit:
  - Cover 138×114×27 and backplate 133.5×109.5×8 each fit the A1 mini's 180² bed with more than 20 mm of margin.
  - On the U1 (270²) both parts fit in one plate stacked in y: 114+109.5 = 223.5.
- The cover prints face-down. The 3 mm 45° front chamfer hides elephant's foot, and the face takes the PEI texture.
- No feature needs supports:
  - The window and key holes are bed-side through-cuts.
  - The vents, USB slot (13 bridge) and snap grooves (1.6 tall) are short bridges.
  - The OLED cable notch (L220) is open-ended.

## 2. Likely criticisms that are overblown or already handled

| Likely complaint | Reality in code |
|---|---|
| "0.8 mm webs between keys are too fragile" | `WEB`=0.8 is the offset *per side* of the diagonal (`sector_up`, L110-119). The real web is **2×0.8 = 1.6 mm**, checked numerically. That is 4 perimeters with a 0.4 nozzle. |
| "Keys will fall out or rattle" | Keys are captive. The flange (r 6.3–16.5, 0.8 thick, L50/L303) overlaps the face by 0.6 inside and 1.0 outside, and 0.2 under each web (`WEB`=0.8 on the hole vs 0.1 on the flange). The switch levers spring it forward. Radial clearance is 0.3. Centre flange r6.0 vs ring flange r6.3 leaves a 0.3 gap, so there is no interlock. |
| "Pre-travel / over-travel undefined" | Nub bottom at 23.5 vs lever top 23.2 gives **0.3** pre-travel. The nub reaches the body (21.2) only after 2.3, which is well past actuation. |
| "Relays/Dupont don't fit 27" | Relays have 1.9 and Dupont has **0.9** of clearance (§1.1). The spec already plans to bend the pins or solder wires. |
| "XL7015 too tall for CTL" | 5.4 clear (§1.2). |
| "Down-switch pins hit F2" | Pins x ≥ 40.6 vs F2 x ≤ 39.5 (L128, L353, L366). The left-switch pins at y −4 clear F2 (y ≤ −8.5) by 4.25. |
| "Relay module too close to the walls" | Module x −63 vs the rim inner face at −65.15 gives 2.15 clear, and y 48 vs 53.15 gives 5.15. The opto/header strip is clear of the OLED frame (x 26..56). |
| "Slab is too big" | The footprint is fixed by a 75 mm off-the-shelf module plus a usable UI column. A larger plate also covers the old Braeburn's paint shadow and screw holes, which a 92 mm square might not. |

## 3. Real weaknesses and the cheap fixes that protect the layout

1. **Coil heat is larger than the spec says.**
   - About 0.35 W per coil, so Heat is about 0.7 W and Cool about 1.05 W, plus XL7015 losses.
   - The XL7015's right edge (x 8) is only **8.5 mm** from the chamber divider, in the same y band.
   - Cheap fixes (no redesign):
     - (a) Add a second 1.2 mm divider skin at x ≈ 19.5 to make an air-gap double wall. It is a cover join only.
     - (b) Add a firmware offset table keyed on relay state. Firmware knows exactly which coils are on.
     - (c) If the CTL layout allows, slide the XL7015 about 6 mm left; the cap could move up.
2. **The wire window barely overlaps a single-gang box.**
   - A box centred under the gang holes has its opening around x 13..69, so the overlap with the window is only about **4 mm** (x 13..17).
   - Fix: add a 0.6 mm breakaway knock-out in the backplate behind the UI column (about x 22..40, y 14..28). The plate is clear there.
   - Keep the foam or TPU plug for bare-hole installs, which is the likely case for a Braeburn swap.
   - Make the gasket a printed **TPU** ring in the existing pocket (L286-288), matching the user's preference for TPU inserts.
3. **The bottom slot's countersink almost touches the sensor rail.** The countersink edge is at y −45.41 and the rail starts at −45.5, a **0.09 mm** gap. Shift the rail (L274-275) −1 mm to give the screw head and driver room.
4. **Fuse reservations look like PCB clips, not uxcell holders.** F1 and F2 are modelled as 22×8×10 boxes (L352-353).
   - The F1 channel (x −65..−18, y −19..−8) fits a Ø10 × about 45 inline holder.
   - F2 can lie at y ≈ −11 under the carrier (13.5 roof). It clears the left-switch pins by about 1.7.
   - Re-reserve both zones now so nobody redesigns around them.
5. **Key flanges overhang 1.3 mm when printed cap-down.** Add a 45° chamfer under the flange. This also adds self-centring.
6. **Key wobble from a single nub.** Add two short fixed stops on the carrier under each ring key's outer flange so the key rocks about a defined line. This is a carrier-only change.
7. **Cover-to-board harness.** About 10 wires (OLED plus 5 switches plus GND) run from the cover to the XIAO. Put a JST-SH/XH at the CTL end so the cover lifts off.
8. **Stale comment.** L128 ("keeps clear of the XL7015") is a v2 leftover. Update it.

## 4. Preserve through any revision
- The side-by-side columns and 27 depth.
- The chamber at the bottom right with its own intake and exhaust path.
- The wire window between CTL and the module terminals (wiring star).
- The tongue-in-groove divider.
- Gang screws in the clear UI column, with the horizontal slot.
- UI-on-cover / power-on-backplate split.
- Snap plus hidden M3 at the bottom.
- Face-down cover print with front chamfer.
- The USB slot alignment.
- Captive flanged keys with 1.6 webs.
