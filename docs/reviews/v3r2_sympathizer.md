# v3 rev 2: sympathizer report

Ground truth is fusion_case_v3.py. All values are in mm, and line numbers are given as Lnnn. ZF = 25.

## 0. Derived numbers used below (from L52-L69)
- LEV_AT_NUB = 3.75·10.1/13.1 = 2.891 → SW_Z1 = 25 − 1.8 − 2.891 = **20.309**, SW_Z0 = 14.309 = CAR_Z1, CAR_Z0 = 13.109.
- CRADLE_TOP = 20.309 + 0.65 = **20.959**; STOP_Z = **22.159**; NUB_Z0 = 23.0; FL_Z0 = 24.2.
- Lever top at the nub = 23.2 vs nub bottom 23.0 → 0.2 preload. Lever tip (s −3) free = 24.06.
- Pin centre hz = SW_Z0 + 1.5 = 15.809 (0.52 above the floor to hole bottom).

## 1. Changes that are genuinely good and must be kept

### 1.1 Switch height and 1.2 travel (L52-L66)
- **Click arithmetic.** The tip/nub ratio is 13.1/10.1 = 1.297. A 1.0 click at the tip is 0.77 at the nub. The 0.2 preload already uses part of that, so the click lands at about **0.57 of key travel**. That leaves 0.63 overtravel at the key (0.82 at the tip) before the legs hit.
- **1.2 vs rev-1's 1.5.** Rev 1 picked 1.5 because the switch height was guessed. 1.2 is the right correction now that the geometry is measured. This is not a regression.
- **Stop legs land on wall tops.** I checked every leg against L417-L427 and L443-L446. Up, left and right are at (±3.65, 11). Down is at local (−3, 7.35/14.65), which after the 180° turn is lateral ±3.65 at s = 3. The centre legs at (39.83, 0) and (45, −5.17) work out to s = 2.0, lateral ±3.65.
  - All of them sit on the 1.2 wall centreline (lateral 3.65).
  - The Ø1.6 leg overhangs the wall by only 0.2 per side.
  - So the press force goes into the cradle, never through the switch.
- **Ghost press is now structurally gone.**
  - Ring flanges at full press sit at 24.2 − 1.2 = 23.0.
  - The centre lever at r ≥ R_IN_FL = 6.3 (s ≥ 6.3) is at most 20.309 + 3.75·3.8/13.1 = 21.4, which is **1.6 below** the pressed flanges. Rev-1 had −0.5 there.
  - Cradle tops (20.96) clear the pressed flanges by 2.0.
  - The rev-1 reliefs (d = 2.0 diagonals, the right-key box at L430) are now belt and braces. They cost nothing, so keep them.
- **Own-flange clearance.** At rest, the lever tip under the key's own flange has 24.2 − (24.06 − 0.26 preload) = **0.4**. When pressed, the tip outruns the flange (1.297×), so the gap grows.

### 1.2 Switch retention: crush ribs + filament pin, lips off (L67-L70, L461-L480)
- **Ribs.**
  - Side ribs: rc = 2.9 + 0.15 + 0.5 − 0.25 = 3.3, so the inner edge is 2.8 vs the body half-width 2.9, giving exactly a 0.1 bite.
  - Wall inner faces are 0.15 off the body, so the ribs do the locating, not the walls. That is the correct way to handle FDM wall bulge.
  - The end rib (se = 11.8, inner edge 11.3 vs body end 11.4) pushes the body onto the tip-end wall (gap 0.05). That fixes the **axial** position of s = 0 under the nub. Since lever height depends on s, this is the rib that matters most. Preserve it.
- **Pin.**
  - The pin sits Ø1.75 in a Ø2.0/2.1 hole, so the switch can lift only about 0.125 before the pin catches.
  - Nothing in service lifts the body anyway: lever reaction pushes it *down* onto the ledges. The pin is anti-fall-out and handling insurance.
  - It is reversible, which the rev-1 "dot of CA" was not.
  - The entry/far radius asymmetry (slip in, press out) is the right idea.
  - Pins go in on the bench into a loose carrier, so finger access is not a cover or backplate problem. Each pin is about 8.5 long (wall outer to wall outer, ±4.25).
- **USE_LIPS False is correct.**
  - Lips plus ribs plus pin is triple retention.
  - Lips would need 1.2 walls 6.65 tall to flex.
  - They would print as unsupported half-round overhangs.
- **Coupling warning.** CRADLE_TOP still contains `+ 0.05 + LIP_R` (L65), and STOP_Z is derived from it. Do not "tidy" LIP_R out of CRADLE_TOP, or the stop moves 0.65 lower and travel silently grows to 1.85.

### 1.3 Solid hull carrier with 3.6 terminal openings (L437, L473, L482-L510)
- **Stiffness.** The 6.65-tall cradle walls on every switch are the ribs. The 1.2 floor between them is mostly in shear. The 3.6×12.4 openings take out the low-stress middle and leave 2.45 of floor each side, 1.2 of which is under a wall.
- **Bed contact.** The hull removes the X's thin spokes and gives a large flat bed footprint (better first layer, no warping arms).
- **Pre-soldered switches.** With 1.1 ledges per side and 0.2 at each end, a switch can drop in already wired. This is a real assembly win with fragile 26 AWG.
- **Clearances.** I checked the hull extents against nearby parts:
  - Up cradle end y 19.75 vs XL7015 y 21.5 (1.75).
  - Down cradle y ≥ −19.25 vs the cap's top surface, which is about 5 below the carrier at y −20.2.
  - Pads at (25, −20) and (57, −18) sit outside the cap's x 30..52.

### 1.4 Window grommet (L71-L80, L365-L368, L375-L397)
- **Volume check: the skirt can disappear.**
  - The protruding skirt is 0.7 × 1.2 = **0.84 mm²** per mm of perimeter.
  - The free groove section beside the in-groove skirt is about 1.2 × (1.8 − 0.29) = **1.81 mm²**.
  - TPU is close to incompressible, so volume is the real test, and it passes with 2× margin. The skirt cannot hold the plate off once it folds.
- **Fold direction.** The stepped flare biases folding outward into the groove. If it goes inward, it goes into the open window (offset < −0.05), which also costs no hold-off.
- **Captive like a real grommet.** The top skirt step (offset +0.075, z 0.9..1.2) is 0.125 wider than the tube. It snaps past the window edge into the counterbore. With the flange on the inside, the grommet stays put while the cable is fed.
- **Prints support-free flange-down.**
  - The membrane is 2 layers on the bed, so the slit is crisp.
  - The tube rises from the flange.
  - The skirt flares 0.125 per 0.3 step (about 23° from vertical).
- **Post notches.**
  - The r3.8 notches clear the r3.5 posts at (11.5, −23) and (8, −1.75) by 0.3.
  - The flange edge at x −19.5 clears the zip bridge (moved to x −22.5..−19.8) by 0.3.
  - Bosses start at x 21.5.
  - Everything was deliberately placed.
- **Primary seal unchanged.** Rev-1 already ruled that foam or putty in the drywall hole is the primary draft seal. The membrane is a baffle, so the long I-slit is acceptable.
- **Keep the 26 slit.** It passes the 18/6 cable plus a possible second C-wire cable. The flaps are 0.4 TPU, about 4 deep, and open with almost no force.

### 1.5 Sensor-cable grommet and cover slots (L84-L87, L293-L294, L366, L398-L407)
- **The best idea in this revision.** SG_SQUEEZE 0.2 × 2 = 0.4 = **SG_SLIT**.
  - The cover slot (4.6) brings the two 2.3 halves exactly into contact.
  - The cover only *closes the slit*. It does not have to bulk-compress TPU.
  - The modelled 8.75 mm³ "interference" is just the open slit being closed. Sliding force is therefore low, so it should not bind or tear.
- **Lead-in.** The 45° lead-in is a 1.2 square rotated about the edge, giving legs of about 0.85, which is 4× the 0.2 per-side squeeze. The slot mouths need no chamfer of their own.
- **Located before the cover arrives.** The block sits in the rib seat with 0.1 clearance up to z 12. It stands upright and stays located while the cable is pressed in through the slit, before the cover goes on.
- **Engagement.** VSKIN engages over 12.5 of slide (16 → 3.5) and VDIV over 5.6 (16 → 10.4), always on narrow 1.2/1.6 strips.
- **Top seal.** The block top meets the slot ceiling at z 16, which gives zero-gap contact at the top.
- **Better than the putty notch it replaced.** It is repeatable, removable and needs no consumables.

### 1.6 Wall mount (L97-L99, L149-L159, L347-L348, L369-L370)
- **Countersink.** It is (4.3 − 2.2)/tan 41° = 2.416 deep, from zt 4.5 down to z 2.08, so there is a **2.08 land**, as the rev-1 synthesis required.
- **Head fits the boss.** The head footprint is x 23.2..51.8, y −17.8..−9.2. The boss is x 21.5..53.5, y −19.5..−7.5. That leaves a 1.7 margin all round.
- **Countersink geometry.**
  - Cones are stepped every 0.5 along the slot, giving a continuous countersunk slot over 65..85 spacing.
  - Printed back-down, the countersink is a funnel opening upward, so it is self-supporting.
- **Slot shank.** It starts at x 25.3, which is 6.5 past the groove edge (18.8). The window and the screw line stay independent.

### 1.7 Rear gasket recess removed (rev-1 L114 → gone)
The old recess was trrect(−24, 24, −27.5, 0.5), 1.0 deep, on the bed face, so printed back-down it had a ~28-wide unsupported ceiling with two islands. Removing it:
- deletes the worst unsupported region on the backplate;
- restores 3.0 under the posts;
- gives a flat, true wall face.

The skirt plus drywall foam does the job the recess was for. **Do not reinstate it.**

### 1.8 Rev-1 rulings: none regressed
| Rev-1 item | Status in this revision |
|---|---|
| m2 SHT40 rail | Present at L343. |
| m9 intake from ix−3 | Present at L304. |
| m11 zip bridge | Present at L349, moved for the flange. |
| m12 ribbed port | Present at L295. |
| Elephant-foot lip | Present at L311. |
| Flange d = 0.18 / 2.0 | Present at L423. |
| Right-key relief | Present at L430. |
| Inserts r2.0 / posts r3.5 | Present at L340-L342 and L359-L362. |
| M4 travel | Deliberately revised; justified in 1.1. |
| m7 CA, gasket recess, putty notch | Superseded by better mechanisms (1.2, 1.7, 1.5). |

## 2. Likely criticisms that are overblown or already handled
- **"The skirt will hold the backplate off the wall."**
  - The volume check (1.4) says it can't once it folds.
  - Two #8 screws 75 apart easily produce the roughly 100 N needed to start the fold (bending stress ≈ 6·F·e/t² with e ≈ 0.375, t 0.7, over about 87 of perimeter).
  - Worst case is a ~2 g TPU reprint or a snip with scissors. The risk is isolated in a cheap part.
- **"The groove ceiling needs supports."** It is a 1.8 cantilever ledge (the "2 mm bridge" comment at L79 is a misnomer), which PETG prints with slight droop. Droop is on a hidden face, and against TPU it only tightens the skirt snap. No change is needed.
- **"The pin is unnecessary given the ribs" or "the pin can't hold."** It is cheap redundancy. The 0.125 vertical slack is irrelevant because nothing in service lifts the body.
- **"The terminal openings weaken the carrier."** No. The walls carry the stop load (see 1.3).
- **"TPU will tear on the cover slot."** The design closes a 0.4 slit with an 0.85 lead-in; no bulk squeeze is involved (see 1.5).
- **"Travel 1.2 is too short to click."** The click comes at about 0.57 (see 1.1).

## 3. Cheap tweaks that protect the strengths
1. **Skirt protrusion.** Set GRM_SKIRT_PROUD from 1.2 to **0.8**, keeping the flare at 1.0.
   - The flare angle goes from about 23° to 27°, so the fold starts more readily.
   - Protruding volume drops to 0.56 mm² against 1.81 free (3× margin).
   - 0.8 of lip is still plenty against painted drywall.
   - This is a one-number change.
2. **Pin holes.**
   - Keep the asymmetry, but set PIN_FAR_R from 0.875 to **0.90**. Horizontal holes print about 0.1 short vertically and droop at the crown, so Ø1.8 nominal is still a press on 1.75 filament without risking a layer split 0.5 above the floor.
   - Optionally make PIN_ENTRY_R 1.0.
   - Cut the pin end at about 45° with flush cutters for lead-in, and keep a 1.8 drill as a fallback.
   - Print one cradle as a test coupon first.
3. **Sensor grommet print orientation (no model change).** Print it **lying on an x end-face**.
   - The channel becomes a vertical Ø2.8 hole and the slit a vertical plane.
   - The lead-in chamfers become vertical edges.
   - The part is 5.8 tall instead of 13 tall with two 2.3 fingers wobbling in TPU.
4. **Window grommet print orientation.** Print it **flange-down**, so the membrane is on the bed. Clean the 0.6 slit with a hobby knife, because first-layer squish can bridge it.
5. **Lever overtravel check (bench, 1 minute).** Measure how far the lever tip goes before the switch bottoms.
   - If it is under about 1.9 above the click-free tip, reduce KEY_TRAVEL to 1.0.
   - Because STOP_Z derives from it, that is a single-number change.
6. **Comment hygiene.**
   - Change L79 from "2 mm bridge" to "1.8 ledge".
   - Add a note at L65 that LIP_R sets the stop height even with lips off.

Items I'd leave alone: the 26 slit, the slot ceiling at z 16 (zero-gap top contact), the d = 2.0 and right-key reliefs, and the solid hull.
