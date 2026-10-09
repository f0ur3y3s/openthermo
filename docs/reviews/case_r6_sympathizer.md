# openthermo enclosure, round 6: sympathizer review

**Reviewer role:** sympathizer (critic and synthesis run separately).
**Inputs:** `fusion_case_v3.py` (ground truth; line numbers below are from this copy), `audit.txt`, `views/01..10`, HARDWARE.md, CLAUDE.md, board_r5_synthesis.md (+ repin drawing), prev_case_v3r3_synthesis.md, dpad_module.py header, CONTROLLER_PARTS_TO_ADD.md, README.md.
**Method:** I recomputed every number below from the script's constants with a short Python check. "Conf." is my confidence in the claim.

**Derived constants used throughout:**

| Constant | Value |
|---|---|
| Cover inner faces | ix 67, iy 55 |
| Backplate rim | outer ±66.85 / ±54.85, inner ±65.25 / ±53.25, top z 8 |
| Plate top | z 3.0 |
| `CTL_TOP` | 7.6 (board z 6.0..7.6) |
| Board (`CTL`, L52) | x −64.18..5.82, y −49.98..−19.98 |
| Hole (c, r) | (−58.39 + 2.54c, −46.41 + 2.54r) |
| D-pad | SFX_Z0 = NUB_Z0 23.35, FL_Z0 24.2, SW_Z1 20.02, SW_Z0 = CAR_Z1 14.02, CAR_Z0 12.82, CRADLE_TOP 20.67, STOP_Z 21.85 |
| Bundle radii | 6 wires 1.83, 15 wires ×0.8 2.32, 10 wires 2.36, 2 wires 1.06, 4 wires 1.50 |

---

## Verdict

**The round-5 board went in cleanly.**
- None of the 64 body-pair overlaps in the audit is a real clash. Every one is a wire landing, a join, a lane envelope, a crush or press feature, or a pin in a switch body (§3).
- Nothing printed touches any reference part that it shouldn't.
- The cradle, the bottom-wall USB cut, the cable plan and the D-pad changes are well reasoned. They meet the hard constraints, and I checked the arithmetic on each.

**I found one real print defect.** The bottom-wall USB cut leaves three floating vent fingers in the cover (audit L73–75). They need supports, which the user has said are not acceptable. The fix is a two-line edit.

**Also worth fixing:**
- **Should-fix:** the centre key's guide-pin holes leave a **0.31 mm** wall at the cap corners.
- **Should-fix (measure):** the board's real outline must be checked against the cradle's 0.15 mm per side.
- **Nice-to-have:** a handful of small items.

**Nothing needs a redesign.**

---

## 1. What is sound, and why

### 1.1 Controller-board cradle (L44–83, L611–635, L549–550): sound. Conf. high

**Board datum.** `CTL` is 70.00 × 30.00 mm. The margins are 5.79 at the left and right and 3.57 at the top and bottom, so the 23 × 9 pitch grid is centred: 23 × 2.54 + 2 × 5.79 = 70.00, and 9 × 2.54 + 2 × 3.57 = 30.00. Every printed feature takes its position from `PB_X0`/`PB_Y0`. Moving the board is therefore one edit, and the pads, fences, pins, pocket and USB cut all follow.

**Support pads** (L82, L612–613): r 1.8, top z 6.0, so the 3 mm underside gap settled in round 3 is kept. I checked each pad against the underside layout:

| Pad | Underside clearance | Notes |
|---|---|---|
| (−62.8, −48.6) | 4.9 to hole (0,0) | corner margin |
| (−38.5, −48.6) | 3.69 from hole (9,0), the start of the C strap, so about 0.9 to its solder fillet | |
| (4.4, −48.4) | x 2.6..6.2; 2.6 from the R pin (23,0) at x 0.03 | |
| (−62.8, −21.4) | 4.9 to (0,9) | |
| (−32.0, −21.4) | 0.57 from the edge of empty hole (10,9) | not a landing |
| (4.0, −24.0) | 1.6 from the R strap on column 23; top at y −22.2, so 0.7 below the grommet flange (y ≥ −21.5, z 3..4.2) | matches the L80 comment |

- Every pad sits under a hole-free margin or an unused hole. None lands on a solder joint or a link.
- Three pads have a cover press pin directly above them: (−38.5, −48.6 / −48.9), (4.4, −48.4) and (−32, −21.4 / −21.5). The board is clamped between pad and pin, so the pins load it without bending it.
- The fourth pin, (4.4, −21.4), is 2.6 mm off its pad (4.0, −24.0). It has to be: the pad must clear the grommet flange. A 2.6 mm offset on 1.6 mm board is harmless.

**Fences and snap beads** (L614–633). Each fence is 1.2 thick with an outer face 0.15 off the board edge, top z 8.45.
- **Bead geometry:** a 0.7071 square rotated 45° makes a diamond with 0.5 half-diagonals, centred on the fence face at z 7.95.
  - It reaches 0.35 over the board.
  - At z 7.6 (the board top) its half-width is 0.5 − 0.35 = 0.15. Its edge lands exactly on the board edge, so it holds the board's top corner with zero vertical play.
- **Print:** both bead faces are at exactly 45°, n.z = ±0.707, which is inside the −0.72 audit threshold. That is why no cradle face appears in the audit's OVERHANG list. The upper face doubles as the snap lead-in.
- **Flex:** each fence is a cantilever about 4.95 tall above the plate (bead at 7.95, plate 3.0).
  - Deflecting it 0.35 gives a bending strain of 1.5·t·δ/L² = 1.5 × 1.2 × 0.35 / 4.95² = **2.6 %**, below PETG yield (about 4–5 %). It is a one-off snap, so that is fine.
  - Stiffness with E = 2 GPa is 14–43 N/mm per fence (lengths 2–6 mm). That gives about 5–15 N of retention each, and about 60 N in total.
- **Flex room:**
  - Bottom fences: their outer face is at −51.33, and the rim inner face is −53.25, so 1.9 of room.
  - Right fences: nothing at z < 9 outside x 7.17.
  - Top fences: they sit at x −58..−40, clear of the grommet flange (x ≥ −19.5).
- **The bottom-left fence near the rim corner** (rim inner arc r 6.25 about (−59, −47)):
  - At x −63.18 the arc is at y −51.65. The fence's outer face is at −51.33, so the gap is 0.32.
  - That is 0.03 short of the full 0.35 deflection, only at the extreme left end.
  - Negligible. I checked it so nobody needs to.

**Left-rim bumps** (L634–635): x −65.45..−64.33, embedded 0.2 into the rim, 0.15 off the board edge, top at 7.6, so they never stand above the board.
- The relay harness lies over the margin at z ≥ 7.77 (centre z 9.6, r 1.83). It clears the bumps' tops.
- It stays inboard of the rim inner face: x −64.43 against −65.25.

**In-plane slack:** 0.15 per side. This is a deliberate choice, and it matches the CLR on the cover rim that was proven on a fit ring.

**Cover press pins** (L83, L549–550): r 1.2, z 7.75 → 25.3, so 0.15 above the board.
- Every pin clears every board part and wire:
  - (−38.5, −48.9): 2.5 from the USB plug lane (x ≤ −42.23), 1.1 from the IN− riser at x −35.53, below the TVS (y ≤ −47.7 against −46.52).
  - (4.4, −48.4): 0.61 from the terminal envelope.
  - (−32, −21.5): 2.3 from the cover branch, 6 from F2.
  - (4.4, −21.4): 1.2 from F2 and 0.6 from the C gutter wire.
- The pins only engage when the cover is on, so they never hinder cover-off service.

### 1.2 Bottom-wall USB cut (L73, L578, L905–906): sound in placement. Conf. high

- **Position:** USB_C = (−48.23, −42.67, 18.7), the face centre of the receptacle (board-frame z 9.5..12.7 → case z 17.1..20.3).
- **Cut:** x −55.73..−40.73, z 14.2..23.2, which is 15 × 9 centred on that point.
- **Plug lane** (x −54.23..−42.23, z 15.2..22.2, from the inner wall at y −55 to y −42.87) is clear of everything, and the audit shows no lane overlap. In particular:
  - The bottom fan lane and the relay harness are at z ≤ 11.4.
  - The trough wires are at z ≤ 12.5.
  - The 1N5819 tops out at z 10.5.
  - The press pin is at x ≥ −39.7.
  - The rim tops out at z 8.
  - The cover inner corner arc only matters at x < −59.
- **Plug reach:** the receptacle face is 12.33 mm inside the inner wall, 14.33 from the outer face. That is less than any USB-C overmould length, so the plug seats with the cover on.
- **Why the bottom wall is the right place:**
  - The cable hangs straight down on a wall unit.
  - The opening faces the floor, so dust and light stay out and it can't be seen from the front.
  - It sits right under the XIAO, so the lane is short and straight.
  - The left wall stays clean.
  - It acts as an extra intake under the controller column.
- **Serviceability:** you must unplug USB before lifting the cover. The plug body crosses the cut edge at z 14.2. That is self-evident and harmless.

### 1.3 Cable routing (L390–487): sound, with good height layering. Conf. high (geometry), medium (real wire dress)

The plan stacks crossing wires in z rather than squeezing them in plan. I checked each crossing:

| Crossing | Vertical gap |
|---|---|
| Cover bundle (z 15.78..20.42) over field R/C (z ≤ 14.8) | 1.0 |
| F2→COM (z 22.2..23.8) over the cover bundle | 1.8 |
| C crossing over R's entry at x 0.03: C at z 13.2..14.8, R at z 11.2..12.8 | 0.4 |
| SHT40 cable (z 3.5..7.5) under the XL IN pair (z 8.24..10.36) | 0.7 |

**Tight but clear threads:**
- **IN+ through the TVS–cap gap.** The run at x −29.4 (r 0.65, so −30.05..−28.75) clears the TVS (≤ −30.34) by 0.29 and the cap box (≥ −28.45) by 0.30.
- **Relay harness past the bottom-left fence:** 0.10 in y and 0.01 in z to the bead apex. These are flexible wires and touching plastic is harmless.

**Antenna:**
- The nearest 24 VAC conductor is the F2→COM riser at x −26.27. The XIAO's PCB edge is at x −39.33, so that is about 13 mm. The field wires are 34 mm or more from the antenna.
- That keeps the board's "14 mm to 24 VAC" intent in the case.

**Fuse service, cover off:**
- F2's top edge is at y −21.09. The cover bundle is 1.47 to the north (y ≥ −19.62). The COM riser is 0.67 left of the holders. Nothing lies over the holders.

**XIAO removal:**
- The cover bundle is 1.05 north of the PCB's top edge. The branch is 1.2 east. The trough wires are below the PCB (z ≤ 12.5 against PCB 16.1).
- So it lifts straight up.

**Field R/C into a downward-facing terminal:**
- The wires come down the right gutter at x 7.0 and 8.8, at z 14 and 12.
- They pass 0.6 outside the (4.4, −21.4) pin and over the fences, which top out at 8.45.
- They run along y −53.2 (1.0 inside the cover wall) and rise into the terminal entries at y −50.8.
- This keeps the screws on top, reachable with the cover off, with no 18 AWG crossing the board.

### 1.4 D-pad module changes (L121–129, L283–302, L742–871): sound, with one exception (§2 S1). Conf. high

**Guide columns and sockets** (L843–846):
- The columns sit at lateral 4.53 from the centre-switch axis, at s = 0. With r 1.5, the column's inner edge is 3.03 from the axis, against the switch's half-width 2.9, so 0.13 clear. The audit's centre-switch crush volume (1.32) equals the arrows', which confirms the columns don't bite the body.
- **Socket:** z 17.45..22.85, r 1.0 (0.125 radial slip for 1.75 filament).
  - The pin bottom sits at 19.35 at rest (2.5 engaged) and 17.85 pressed (0.4 above the socket floor).
  - The socket floor is 0.88 above the top of the retaining-pin hole (15.52 + 1.05 = 16.57).
- The column tops (STOP_Z 21.85) are the centre key's stop: 23.35 − 21.85 = 1.5 = KEY_TRAVEL.
- The landing ring is r 1.0..1.5, about 3.9 mm² per column. A 20 N press gives about 2.5 MPa, which is trivial.
- The centre retaining pin (reach 7) notches the column bases at z 14.5..16.6. That leaves about s −1.5..0.7 of section, which is ample for a compression stop.

**Pillars** (L847–855): r 0.8, centred on the 1.2 cradle wall (so 0.2 per side past the wall faces), on a 45° cone from r 0.6 at z 20.17 to r 0.8 at z 20.37.
- **Support-free:** the overhang beyond the wall faces rises at exactly 45°, which is correct.
- **Placement:**
  - Arrow pillars are at lateral 3.65 and s = 0.
  - The down key's pillars are at (44.0, −11.35) and (44.0, −18.65), on its two side walls at s 3.0.
  - Both lie under their key flanges, outside the lever pockets. The down key's pillars are 0.6 from its ±2.25 pocket.

**Post rings** (L837–838): the carrier ring is r 3.6, equal to the clamp tube, so no lip, and nothing for Orca to support.

**Full-height wire slits** (L782–795):
- **Arrows:** straight through the end wall. The end crush ribs are split at ±1.9, which leaves 0.6 to the slit.
- **Centre:** angled from (48.42, 3.42) to (50.90, 17.21). At y 10.1 the slit's edge is at x about 50.4, 1.1 from the post tube (51.5).
- **Down:** x 45.7..47.3. It is 0.9 from the pin hole (48.2), 0.9 from the pillar (44.8), and between the s 2.0 and 8.5 ribs.
- **Value:** a wired switch slides out sideways. That is a real serviceability gain for the cost of some cut plastic.

### 1.5 24 VAC separation: sound. Conf. high

- **24 VAC is confined to:**
  - the window column (x −6..4);
  - the right gutter (x 7–9);
  - the bottom-right terminal;
  - the F2 / COM riser at x −26.27, then z 23 over the relay module.
- **Logic is confined to:**
  - the left lane (x ≤ −60.8);
  - the XIAO trough;
  - the cover bundle at z 15.8–20.4, kept at least 1.0–1.8 apart in z wherever it crosses 24 VAC.
- Everything is insulated Class 2 wire. C is GND by design (CLAUDE.md).
- The 1 mm fuse pocket (L74, L586–587: x −25.8..3.0, y −42.25..−20.09, z 24..26, leaving 1.0 skin) covers the side-by-side F1/F2 holders (x −24.8..2.0, y −41.25..−21.09) with 1 mm all round. It opens upward when printed face-down, so it needs no support.

### 1.6 SHT40 chamber (L164–175, L636–639, L544–548): sound. Conf. high

**Module position:** x 40..52.56, y −50.5..−40, z 14..16.6.
- Pin edge 19.9 from the double skin (x 20.1).
- 14.4 from the outer wall.
- 11 off the plate.
- Under the exhaust port's x range (28..54).
- In the vent band, which runs z 7..20 on both the bottom and right intakes.

**Supports:**
- **Standoff** at (50.16, −42.7), r 2.0: inside the module footprint by 0.7 (y) and 0.4 (x). Its x max of 52.16 clears the screw boss (x ≥ 53) by 0.84, as the comment claims.
- **Posts:** r 1.2, inside the footprint by 0.3.

**Cable:** passes the TPU block in a channel at z 6, r 1.8 (Ø3.6, which holds a Ø2.99 four-wire bundle).
- It then runs at x 11, z 5, under the XL IN pair and the field wires, to the right gutter.
- **Grommet fits** (both intentional):
  - squeezed 0.15 per side by the cover's slots: audit L2, 3.24 mm³;
  - pressed 0.1 per side into the rib seat in y: audit L4, 5.38 mm³.

**Double skin:**
- The cover tongue at x 15.7..17.3 sits 1.6 from the skin at 18.9.
- The skins stop 0.5 above the plate, and 0.5 above the rim where they cross it (z 8.5).

**Thermal:** the hot sources (relays at top left, the controller at bottom left) are on the other side of the double skin.

### 1.7 Print orientation and support-free features: sound, apart from 2.1. Conf. high

| Audit OVERHANG lines | What they are | Verdict |
|---|---|---|
| L68–72 | 0.4 mm elephant-foot reliefs at the key openings | intentional, single perimeter |
| L76–123 | 2 mm vent bridges | fine |
| L126–131 | 7.4–8.1 mm exhaust-port bridges in a 1.2–1.6 wall | short bridges, fine |
| L67 | Ø3.4 M3 clearance hole in the bottom wall | self-bridging |
| L124–125 | snap-groove ledge, 1.25 deep, at z 4.2 | already printed on the fit ring; see N4 |
| L132 (backplate) | 0.4 rim foot relief | trivial |
| L133 (backplate) | 1.8 mm grommet-groove ledge | documented choice (L157) |
| L134–138 (backplate) | 5 mm tie-tunnel bridges and the 5.5 mm tie-bridge roof | fine |

None of the new cradle features (pads, fences, beads, bumps) is flagged. That is exactly what the 45° beads were designed to achieve.

---

## 2. Recommended changes

### Blocker (print)

**B1. The USB cut leaves three floating vent fingers in the cover's bottom wall. Conf. high**

- **Evidence:**
  - The audit flags overhangs at x −45..−42, −50..−47 and −55..−52, all at z 14.2 (audit L73–75, 6.0 mm² each).
  - The intake vents (L574–575) are at cx −56, −51, −46, −41: slots [−57,−55], [−52,−50], [−47,−45], [−42,−40], z 7..20.
  - The USB cut (L578) spans x −55.73..−40.73, z 14.2..23.2.
  - Inside the cut's x range, the wall below z 14.2 is left as three 3 × 2 mm fingers, at x −55..−52, −50..−47 and −45..−42. Each runs z 7..14.2 and is joined to the wall only at its z 7 end.
  - View 08 shows them: the stepped line under the cut.
- **Why it matters:** the cover prints front-face down, so z falls as the print rises. Each finger's first layer, at z 14.2, starts 12.8 mm above the bed with nothing below it, because the cut is open from z 14.2 to 23.2. They are true islands, so Orca will either add supports or print three strings. The user has said that is not acceptable.
- **Fix (2 lines, at L574):** skip the four intake vents that touch the cut:
  ```python
  for cx in list(range(-56, 11, 5)) + list(range(21, 51, 4)):
      if abs(cx - USB_C[0]) < 7.5 + 1.0 + 0.5:      # cx -56, -51, -46, -41: keep the USB cut a clean rectangle
          continue
  ```
  - The test is |cx + 48.23| < 9.0, which removes exactly −56 (7.77), −51 (2.77), −46 (2.23) and −41 (7.23). The next vents, −61 (12.77) and −36 (12.23), are kept.
  - The cut's print-top edge at z 14.2 then becomes one **15.0 mm bridge** in a 2 mm wall, anchored on solid wall at both ends. That is a short bridge, within the brief's allowance.
  - **Airflow:** this removes four 2 × 13 slots (about 104 mm²). The cut itself adds 135 mm² of intake, so the controller column's intake area does not go down.
- **Alternative**, if the user wants no bridge at all: make the cut a gable in the print direction, with its z 14.2 edge as a 45° roof peaking at the centre. Not needed for PETG at 15 mm.

### Should-fix

**S1. The centre key's guide-pin holes leave a 0.31 mm wall at the cap corners. Conf. high (geometry), medium (consequence)**

- **Evidence:**
  - The guides are at DC + (∓3.203, ±3.203): `ck_guides()`, L283–287, with CK_GUIDE_P 4.53 / √2 = 3.203.
  - The cap is `sq_shape('center', 0)` (L293): half 4.5 with corner r 1.5, so the corner-arc centre is at (3.0, 3.0).
  - The hole centre is 0.287 from the arc centre. The wall is 1.5 − 0.287 − 0.9 (CK_HOLE_R) = **0.31 mm**.
  - This runs over the cap's height inside the hole: z 25.0 → CK_HOLE_TOP 27.2, 11 layers at 0.2.
  - In the flange below (z 23.35..25: half 5.3, r 2.3), the wall is 1.11. That part is fine.
- **Why it matters:**
  - 0.31 mm is below the 0.4 nozzle. Arachne will either drop it, leaving an open slot at a visible corner seen through the 0.3 opening clearance, or print a single sliver.
  - The 1.75 filament pin is a snug fit in a printed Ø1.8 hole, which typically prints about 1.65. Pressing it in loads the corner in hoop stress, so a split is likely.
  - This feature is new since round 3.
- **Why moving the pins doesn't help:** the pins can't move inward. At P = 4.53 the column's inner edge is already only 0.13 from the switch body (§1.4).
- **Fix (one constant):**
  - Stop the blind hole at the flange top: `CK_HOLE_TOP = ZF` (L128).
  - Then `CK_PIN_CUT` = 25.0 − 23.35 + 4.0 = **5.65** (it was 7.85). It recomputes automatically. Update HARDWARE.md, which says "7.9 mm", and the dpad_module docstring.
  - The pin then sits 1.65 mm deep in the 1.11 mm-walled flange. Fix it with a drop of CA glue.
  - The guiding engagement in the sockets (2.5 at rest) is unchanged.
- **Check before changing:** if the Orca preview of the key plate at 0.2 mm layers shows a solid 2-line wall at those corners, keep the current hole.

**S2. Measure the perfboard outline and the grid offset before printing the backplate. Conf. high (that it is unmeasured), medium (that it matters)**

- **Evidence:**
  - The cradle allows 0.15 per side: left bumps at x −64.33 to right fences at 5.97 is 70.30, and bottom to top fence faces is 30.30.
  - That assumes the board is exactly 70.00 × 30.00 with the grid centred (the 5.79 / 3.57 margins in L52).
  - HARDWARE.md and build step 1 list neither as measured. Cut perfboards commonly vary ±0.2–0.3 and are often not centred.
  - An oversize board won't drop past the beads. An off-centre grid moves the XIAO, and so the USB face, relative to the cut (±7.5 / ±4.5 of margin, so that tolerates a lot), and it moves the pads relative to the holes. The smallest pad clearance is the (−32, −21.4) pad, 0.57 from the edge of hole (10,9), and that hole is empty.
- **Fix:**
  - Add "perfboard outline (L × W) and the edge-to-first-hole distances" to build step 1 and to CLAUDE.md's "measure before printing".
  - Put the measured values into `CTL`, as `PB_X0 − m_left` and so on.
  - If the board is more than 70.25 or 30.25, sand it to 70.0 / 30.0. That is easier than reprinting.

**S3. Keep the XL7015 IN pair inboard of the rim line. Conf. medium (a wire-dress item; reference only)**

- **Evidence:**
  - The IN pair runs at y −53.4, z 9.3, r 1.06 (L444–446), so y −54.46..−52.34 and z 8.24..10.36.
  - The run is 43 mm long (x 13.5 → −29.4).
  - It lies 0.24 above the backplate rim top (z 8) and reaches 0.39 from the rim's outer edge (−54.85). The cover wall slides down past that edge with a 0.15 gap.
  - This is the same pinch mode round 3 accepted for the left harness (M7). Here it carries the 35 V bus, so a chafe would blow F1. That is a safe outcome, but a nuisance.
  - The route sits that far out to clear the cap's **box** (y ≥ −51.81). The real can is a cylinder about (−46.81, z 12.6) of r 5, so at z 9.3 it only reaches y −50.56.
- **Fix:**
  - Route the pair at **y −52.2, z 9.6** (L444, L446, L448: `-53.4` → `-52.2` and `9.3` → `9.6`). Both values are needed.
    - At y −52.2 alone, the pair's inner edge (−51.14) would cross the bottom-right fence (x 3..5, y −51.33..−50.13, top 8.45) by 0.19 in y and 0.21 in z.
    - Raising it to z 9.6 puts its underside at 8.54, which is 0.09 above that fence.
  - **Resulting clearances:**
    - The outer edge is at −53.26, inboard of the rim inner face.
    - The real can reaches y −50.81 at z 9.6, so the pair clears it by about 0.3.
    - It clears the terminal (−50.41) by 0.7.
    - Field R's underside is at 11.2; the pair's top is at 10.66, so 0.5.
  - Add a build note: tack it to the rim's inner face with Kapton.
  - If the model's box cap then flags an overlap, that is an artefact of the cap box. Model the cap as a cylinder (r 5, axis y −46.81, z 12.6) instead.

### Nice-to-have

**N1. Delete the stale grommet-flange clearance at (11.5, −23). Conf. high**

- **Evidence:** L678 still cuts the grommet flange with a r 3.8 circle at (11.5, −23). That was the old controller board's screw post, which no longer exists. The cut removes the flange at x 7.7..15.3 down to y −19.2, which leaves **0.2 mm** of flange beside the window there.
- **Fix:**
  - L678: `for cx, cy in ((8.0, -1.75),):` (keep the relay-module boss).
  - I checked the restored flange against what is nearby:
    - the SHT40 cable is at z ≥ 5.2 there;
    - the (4.0, −24) pad top is −22.2;
    - the right fence top is −22.98;
    - the XL IN pair is at z ≥ 8.2.

  All of these clear the flange (z 3..4.2, y ≥ −21.5).

**N2. Dress the logic wires about 3 mm off the XIAO antenna end. Conf. medium (RF effect small)**

- **Evidence:** the antenna end is x −57.13..−39.33, y −25.67..−20.67, z about 16–17.
  - The cover bundle (r 2.32) runs at y −17.3, so 1.05 from it at the same height.
  - The branch at x −36.8 descends 1.2 from the PCB's side edge alongside the antenna zone.
  - These are logic wires (3V3, I2C, keys, 5 V/GND), not 24 VAC, but copper this close still loads a chip antenna.
- **Fix (reference only):**
  - Run the bundle at **y −15.0** instead of −17.3 (L476, L480–482). Its edge is then 3.4 from the antenna.
  - I checked the path: the field risers are at z ≤ 13.8, the relay terminals at y ≥ −4, the COM jumpers at y ≥ −7.1, the tie bridge at z ≤ 8.3, and the F2→COM wire at z ≥ 22.2. Nothing is in the way.
  - Move the branch drop to x −35.0. It then clears the (−32, −21.5) pin by 0.5.
  - Add an HARDWARE.md line: "dress no wire over the XIAO's top 5 mm".

**N3. Fuse-pull lift. Conf. medium**

- **Evidence:** pulling a fuse with the cover off loads the board upward against bead retention.
  - The right-edge fences are short (3 and 4 mm, about 7–10 N each).
  - The bottom-right fence is 2 mm long, about 5 N.
  - Pulling a fuse takes about 5–15 N.
- **Fix:** no model change needed. Add a build note: "hold the board down beside the holder (or use a fuse puller) when swapping a fuse".
- **Optional:** lengthen the bottom-right fence to x 3..5.5. It has 0.41 to the terminal at x 2.59, so it can't grow leftward. Gain is marginal.

**N4. Snap-groove ledge (audit L124–125). Conf. low that it needs action**

- The 1.15–1.25 mm ledge at z 4.2 prints as a short overhang and was evidently accepted on the fit ring. No action.
- If it ever curls, make the z 4.2 face a 45° ramp. That softens retention, but the bottom M3 holds the cover.

**N5. Docs and comment nits. Conf. high**

- **Code comments:**
  - L173–174 and L462 still say the SHT40 cable passes "under the 470 µF's overhang". It runs at x 11, right of the board; the cap is at x ≤ −8.45.
  - L614 assigns `zt_` twice; the first value is dead.
- **README.md is stale:** heat-set inserts, a 65–85 slot range, "Wall mount slots horizontal".
- **CONTROLLER_PARTS_TO_ADD.md is stale:**
  - it says the USB goes "through the left wall (the existing cut)";
  - its screw-hole table describes the old board.
- **Pin length:** HARDWARE.md and the dpad_module docstring say the centre guide pins are 7.9 mm. Change that to match S1 if it is adopted.

---

## 3. Audit overlap classification (all 64 pairs)

| Audit lines | Pair | Class | Real problem? |
|---|---|---|---|
| L2 | Cover × sensor grommet 3.24 | Intentional: the 0.15/side squeeze of the cover's slots (L554) plus the 1 mm slot-mouth chamfers | No |
| L3 | Cover × OLED PCB 1.20 | Intentional: 4 crush bumps, each 0.3 into a 0.15 gap → 0.15 × 2 × 1.0 × 4 = 1.2 | No |
| L4 | Backplate × sensor grommet 5.38 | Intentional: seat 0.1/side narrower in y (L659); press fit | No |
| L5–10 | Window grommet × field Y1, G, O, W, R, C 0.80 each | Intentional: wires through the slit membrane (z 3.8..4.2) | No |
| L11–15 | Carrier × switch bodies 1.32 (down 1.07) | Intentional: crush ribs with 0.1 bite (down has one end rib, not two) | No |
| L16–21 | Relay input terminal × harness fans | Wire landing | No |
| L22–25 | XL7015 board/parts × OUT−, OUT+, IN+ | Wire landing. The 0.05 graze is the crude parts box | No |
| L26 | JST 6p × D-pad (6) | Wire landing | No |
| L27 | SHT40 × SHT40 (4) | Wire landing | No |
| L28–29 | OLED PCB / back parts × OLED (4) | Wire landing | No |
| L30–34 | SW bodies × 1.75 retaining pins 13.95 | Intentional: the pin passes through the switch's mounting hole (the body is modelled solid) | No |
| L35–40 | Field Y1/G/O/W mutual | Intentional: one cable lane drawn as coincident tubes (L403, `wy − i*0.0`) | No |
| L41–43 | F2→COM1 × COM jumper; jumper × jumper | Wire joins at COM terminals | No |
| L44–49 | Relay harness × fans 1–6 | Bundle-to-fan joins | No |
| L50–51 | Relay harness × left lane / bottom fan | Lane envelopes containing their wires | No |
| L52–61 | XL7015 harness/pairs mutual (incl. OUT+ × IN+ 17.91) | Bundle splits and joins. OUT+ and IN+ share a drawn path at x 14 (L452–454) | No |
| L62–66 | SHT40 / XL OUT / OLED pair × controller side, cover bundle, branch | Wires joining their bundle | No |

**Result: 0 real interferences.** In the OVERHANG section only L73–75 are real (B1). Classification of the rest is in §1.7.

---

## 4. Summary table

| # | Severity | Item | Edit | Conf. |
|---|---|---|---|---|
| B1 | Blocker (print) | Floating vent fingers under the USB cut | L574: skip vents with \|cx − USB_C.x\| < 9 → 15 mm bridge | High |
| S1 | Should-fix | 0.31 mm wall at the centre-key guide holes | `CK_HOLE_TOP = ZF`; pin 5.65 long + CA; or verify in the slicer first | High / med |
| S2 | Should-fix | Perfboard outline and grid offset unmeasured (0.15/side cradle) | Measure; feed into `CTL`; sand if oversize | High / med |
| S3 | Should-fix | XL IN pair 0.39 from the rim pinch edge | y −53.4 → −52.2 **and** z 9.3 → 9.6 (L444–448); tack with Kapton | Med |
| N1 | Nice | Stale (11.5, −23) flange cut | L678: keep only (8.0, −1.75) | High |
| N2 | Nice | Logic wires 1 mm from the antenna end | Bundle y −15.0; branch x −35.0 | Med |
| N3 | Nice | Fuse pull can lift the board | Assembly note | Med |
| N4 | Nice | Snap-groove 1.25 ledge | None, unless it curls | Low |
| N5 | Nice | Stale comments and docs | See list | High |

**Everything the brief asked about holds up arithmetically:**
- the cradle;
- the bottom-wall USB placement and lane;
- the cable layering;
- the D-pad pillars, rings, slits and sockets;
- the 24 VAC zoning and fuse pocket;
- the SHT40 chamber.
