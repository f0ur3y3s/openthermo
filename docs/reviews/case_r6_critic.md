# openthermo enclosure, round 6: CRITIC

**Inputs read:** BRIEF.md, fusion_case_v3.py (all 964 lines), audit.txt, HARDWARE.md, CLAUDE.md, board_r5_synthesis.md, prev_case_v3r3_synthesis.md, CONTROLLER_PARTS_TO_ADD.md, README.md, dpad_module.py, and all 10 views.

**Method:**
- Every number below is recomputed from the script.
- Wire-to-wire and wire-to-part gaps come from a standalone pure-Python copy of `build_wiring()` and the relevant boxes, in `scratchpad/critic_work/geo.py` and `boxes.py`. Gap = axis distance minus radii; negative means overlap.
- Frame: ix = 67, iy = 55, backplate rim outer face y = −54.85, rim inner face y = −53.25, rim top z = 8, CTL_TOP = 7.6, ZF = 25.
- Board in the case: x −64.18..5.82, y −49.98..−19.98.
- XIAO PCB: x −57.13..−39.33, y −41.67..−20.67, z 16.1..17.1. The antenna is at the y −20.67 end.

**One context point that changes several findings:** the backplate goes on the wall first, so the board, the harness and the cover are all fitted with the case vertical. Gravity points toward **−y, the bottom wall**. Floppy 26 AWG bundles sag toward the bottom rim and toward the board's top edge, and that is where the new routes have their tightest gaps.

---

## Summary table

| # | Severity | Finding | Confidence |
|---|---|---|---|
| B1 | **Blocker** | The USB cut merges with 4 intake slots and leaves 3 free-floating fingers that print in mid-air | High |
| S1 | Should-fix | The XL7015 IN pair (bus + C) lies on the bottom rim top, 0.39 mm from the rim's outer edge, on the cover's pinch line | High (geometry), medium (that it actually gets pinched) |
| S2 | Should-fix | The cover bundle and branch wrap the XIAO's antenna end at 0.2–1.3 mm, and gravity sag pushes the bundle into the (−32, −21.5) press-pin path | High (geometry), medium (RF impact) |
| S3 | Should-fix | Centre-key guide-pin hole leaves a 0.31 mm wall in the cap corner (0.40 at best on the flat sides); the press-fit pin will split it | High |
| S4 | Should-fix | The USB pull-out load goes into two thin fences, 1.2 mm thick (one only 2 mm long), stressed across the layer lines | Medium |
| S5 | Should-fix | In the right gutter, the SHT40 (I2C/3V3) cable is squeezed between unfused field R and the bus pair: overlaps −0.087 and −0.051 mm | High (geometry), low–medium (hazard) |
| S6 | Should-fix | "Controller side (10)" bundle passes 0.2 mm under the carrier's left cradle corner, in the cover's insertion path | High (geometry) |
| S7 | Should-fix | The cradle assumes symmetric board margins (5.79 / 3.57) and a 1.6 mm board; nothing measured | Medium |
| N1–N9 | Nice-to-have | See the list at the end | — |

**Checked and not found to be a problem:**
- **The 74 audit overlaps.** I classified every one: each is a landing, a join, a lane envelope, a press or squeeze fit, a clip, or a pin in a switch. **None is a hidden real interference.** The three real contacts the audit misses (S5, and the box-model TVS contact in N7) are below its volume threshold.
- **Fuse swap** with the cover off.
- **XIAO removal.**
- **24 VAC distance to the antenna.** It is at least 12.3 mm: the COM riser at x −26.27 against the antenna-zone edge at x −39.33.
- **SHT40 heat.** No new heat source near the chamber.
- **New backplate cradle features.** All support-free (45° beads, boxes, cylinders).
- **Cover fuse pocket.** Opens upward in print.

---

## B1 (Blocker): the USB window plus intake slots leaves three floating fingers

**Evidence:**
- L578: the USB cut is x `USB_C[0] ± 7.5` = **−55.73..−40.73**, z 14.2..23.2, through the bottom wall (y −58..−54).
- L574–575: the bottom intake slots `range(-56, 11, 5)` are each cx ± 1, z 7..20. The slots at cx = −56, −51, −46 and −41 (x −57..−55, −52..−50, −47..−45, −42..−40) cross the USB window.
- That leaves wall material at **x −55..−52, −50..−47 and −45..−42**, z 7..14.2. Each piece is joined only at z < 7 and bounded on both sides by slots, so it is a free-standing finger with its free end at z 14.2. View 08 shows the comb.
- The audit lists exactly these three faces: `OVERHANG Cover n.z=1.00 area=6.0mm2 [-55..-52 | -50..-47 | -45..-42, -57..-55, z 14.2]`.

**Why it fails:**
- The cover prints face-down, so z = 14.2 is the first layer of each finger.
- Each 3 × 2 mm face has no anchor at that layer: slots are on both sides and the window is above it. It is an island printed on air, not a bridge.
- Orca will either generate supports, which the user forbids, or print spaghetti that the following layers build on.
- Even if it printed, a 3 × 2 × 7.2 mm PETG finger next to a USB plug is fragile.

**Fix:** skip the intake slots that touch the USB window. In the L574 loop:

```python
for cx in list(range(-56, 11, 5)) + list(range(21, 51, 4)):
    if abs(cx - USB_C[0]) < 10.5:        # -56, -51, -46, -41 overlap the USB cut
        continue
```

- The window ceiling in print is then a single 15 mm bridge, 2 mm deep, anchored at both ends. That is acceptable.
- The board column loses 4 of its 14 bottom slots (4 × 2 × 13 = 104 mm²). The 15 × 9 = 135 mm² USB window replaces that area.
- Re-run the audit and check that the three z 14.2 faces are gone.

**Confidence:** high.

---

## S1 (Should-fix): the XL7015 IN pair lies on the cover/rim pinch line

**Evidence:**
- L444–445 route the IN pair at y **−53.4**, z 9.3, r = `r_bundle(2)` = 1.057, from x 13.5 to −29.4.
- So the bundle spans **y −54.46..−52.34** and **z 8.24..10.36**.
- The backplate rim is y −54.85..−53.25, top at z 8. The pair therefore sits 0.24 mm above the rim top, with **0.39 mm** to the rim's outer edge. The cover's inner wall at y −55 is 0.54 mm away. My checker gives XLIN2–RimBot 0.243 and XLIN2–CoverWall 0.543.
- This is 43 mm of floppy 26 AWG, and the case is vertical during assembly, with gravity toward −y. The pair will lie on the rim top and drape over its outer edge.
- When the cover is pushed home, its wall slides down the rim's outer face with 0.15 mm clearance and shears or traps the pair.
- The result is bus (33–39 V) shorted to C. F1 blows, which is safe, but the unit is dead and the bus wire is damaged.
- The round-3 M7 ruling covered the same mechanism on the left rim. This is a new instance created by today's route.

The field R and C runs at y −53.2 (L407–410) are 18 AWG solid at z 12 and 14, with their outer edge at y −54.0. They hold their shape, so they are lower risk. They still sit 0.85 mm inside the rim edge.

**Fix:**
1. Move the IN pair inboard. At L444 use `(13.5, -16.0, 9.3), (13.5, -52.3, 9.3), (-29.4, -52.3, 9.3)`, and the same y −52.3 in the IN− and IN+ starting segments (L446–448).
   - Outer edge −53.36, which is 1.5 mm inside the rim edge.
   - Clearance to the 470 µF: at z 9.3 the can's surface is at y −46.81 − √(5.1² − 3.4²) = −50.61, giving 0.63 mm.
2. Add two or three cable clips to the backplate, hooks on the rim's inner face at z 8–12, for example at x −20, −5 and 10. Or note in the build steps: "tape the IN pair and R/C to the rim inner face with Kapton before closing".
3. Add "look along the bottom rim before closing" to the build order.

**Confidence:** geometry high. That it actually gets pinched: medium.

---

## S2 (Should-fix): the cover bundle wraps the XIAO antenna end, and its sag lands in a press-pin path

**Evidence:**
1. **Cover bundle (L480):**
   - It runs y −17.3, z 18.1, r = `r_bundle(15) * 0.8` = 2.316, from x 10 to −58.5. That spans y −19.62..−14.98 and z 15.8..20.4.
   - The XIAO PCB ends at y −20.67 (z 16.1..17.1), so **15 parallel conductors (3V3, SDA, SCL, keys) run 1.05 mm from the antenna end, at the antenna's height**, across the whole XIAO width.
   - The drop corner at (−58.5, −19.6, 13.5) is **0.21 mm** from the XIAO PCB corner (checker: CoverB–XIAO_PCB 0.208).
   - The model also shrinks the bundle by 0.8. A real 15 × 26 AWG bundle (OD ≈ 1.3) is about Ø5.8 (r 2.9), which closes the gap to about 0.5 mm.
2. **Cover branch (L482):** at x −36.8, r 1.29, so x −38.09..−35.51. It runs y −17.3 → −28, alongside the antenna zone (y −25.67..−20.67) at **1.24 mm** from the PCB's right edge.
3. **Column-0 landings:** the column-0 wires at x −58.39 run past the PCB's left edge (−57.13) at 1.26 mm, and rows 8–9 sit inside the antenna zone's y-span.
4. The antenna zone is therefore bordered on three sides by copper within about 1.3 mm. CONTROLLER_PARTS_TO_ADD.md (clearance 1) asks for 5–10 mm with no metal.
5. **Pinch:** with the case vertical, the bundle sags −y. The press pin at (−32, −21.5), r 1.2 (L83, L549–550), spans y −22.7..−20.3, which is only **0.7 mm** from the bundle's modelled edge. A 1 mm sag puts the bundle under the pin. When the cover goes on, the pin drives the bundle down onto the board margin at z 7.75. The cover won't seat, or the wires are crushed.

**Fix (model and routing):**
1. **Cover bundle run (L480):** `(10.0, -13.0, 19.5) → (-62.0, -13.0, 19.5) → (-62.0, -24.0, 15.0)`, then fan into column 0 from the left.
   - At y −13, r 2.3, it is 5.4 mm from the XIAO end (5.1 mm with the real r 2.9).
   - At z 19.5 its top is 21.8, under the COM wire's underside at 22.2, and its bottom of 17.2 is above the field wires (top 13.8).
   - It stays at y ≤ −10.7, clear of the relay K terminals (y ≥ −4).
   - The drop at x −62 sits above the relay harness (z ≤ 13.3).
2. **Branch:** move it to x −35.5. That gives 2.5 mm to the PCB and 1.0 mm to the (−32, −21.5) pin.
3. **Anchor:** add a tie point so the bundle cannot sag onto the board's top edge. For example, a zip-tie anchor on the backplate at (−30, −11), clear of the left wall-screw boss (x −54..−26, y −19.5..−7.5): put it at (−24, −11) or on the boss's top edge.
4. **Bench:** keep the U.FL / FPC fallback in mind. Log RSSI with the cover on before the final install.

**Confidence:** geometry high. RF impact medium: it is unmeasured, but 1 mm of parallel copper at a chip antenna's end is a known detuner. The pinch is medium.

---

## S3 (Should-fix): the centre-key guide-pin hole leaves a 0.31 mm wall in the cap

**Evidence:**
- L125–129, L283–287 and L296–297.
- **Guide positions:** `ck_guides()` puts the guides at DC + (∓3.203, ±3.203): (37.797, −0.797) and (44.203, −7.203). That is 4.53 from DC on the cross-diagonal.
- **Hole:** r `CK_HOLE_R` 0.90, from SFX_Z0 23.35 up to `CK_HOLE_TOP` 27.2, so 2.2 mm of it is inside the **cap** (z 25..27.2).
- **Cap:** half 4.5 with corner r 1.5 (L98, L257), so the corner arc centre sits at 3.0·√2 = 4.243 from DC and the cap's boundary on that diagonal is at 5.743.
- **Hole outer edge:** 4.53 + 0.90 = 5.43, which leaves a **wall of 0.313 mm**.
  - Even with a sharp corner, the hole is 1.297 − 0.9 = **0.397 mm** from the flat sides x = DC − 4.5 and y = DC + 4.5.
  - The flange (half 5.3, r 2.3) gives 1.11 mm, so only the cap portion is thin.
- The fit is snug: a 1.75 filament (r 0.875) in r 0.90, and printed holes shrink by about 0.1, so it ends up as a press fit.
- Orca's Arachne will print one bead about 0.31 wide, 2.2 mm tall, around a press-fitted pin. That splits on insertion, or bulges outward. The cap corner then rubs the 0.3 mm opening clearance (L581), and the centre key sticks. The top 0.2 mm (z 27..27.2) is above the cover face, so a split is visible.
- **Why the obvious fix doesn't work:** moving the guides inward does not work. A column r 1.5 must stay at least 4.5 from the switch axis (body half-width 2.9, plus 1.5, plus 0.1), while the hole must stay at most about 4.0 from DC to keep 0.8 of wall. The current geometry cannot satisfy both.

**Fix (pick one):**
- **(a) No new parts:** end the blind hole in the flange. Set `CK_HOLE_TOP = ZF` (wall 1.11) and `CK_PIN_LEN` unchanged, so the cut length is 25 − 23.35 + 4 = 5.65. Fix the pins with a drop of CA. That gives 1.65 mm of engagement, which is short but no longer splits anything.
- **(b) Better:** use a 1.0 mm steel wire (paper-clip) guide.
  - `CK_HOLE_R` 0.55, `CK_SOCK_R` 0.62, `CK_COL_R` 1.2, `CK_GUIDE_P` 4.2.
  - Hole edge at 4.75 from DC: about 1.0 to the cap arc and 0.75 to the flat sides.
  - Column inner edge 4.2 − 1.2 = 3.0, against the body at 2.9.
  - Socket wall 0.58.
  - Steel will not snap, which was the reason for filament.
- **(c)** Grow `SQ_C_HALF` to 4.8 to gain 0.3 of wall. This eats the face web (1.0, and 0.2 at the elephant-foot relief), so it is not recommended.

**Confidence:** high on the geometry. Medium-high that it cracks on a press fit.

---

## S4 (Should-fix): the USB pull-out load goes into two thin fences

**Evidence:**
- Retention in −y is only the two bottom fences (L616): ('x', −63.18..−57.18) and ('x', **3.0..5.0**).
  - Each is 1.2 thick, rooted at the plate top (z 3) and standing to 8.45.
  - The board edge bears on it at about z 6.8, an arm of about 3.8 mm.
- The press pins have a 0.15 gap (L550), so they give no friction.
- USB-C unmating force is 8–20 N. At the plug line x −48.23, the left fence carries (4 + 48.23) / (4 + 60.18) = 81 %, about 12 N at 15 N pull.
- That gives M = 12 × 3.8 = 46 N·mm, Z = 6 × 1.2² / 6 = 1.44 mm³, so **σ ≈ 32 MPa**. The stress runs across the layer lines (the backplate prints back-down), where PETG holds about 25–35 MPa.
- The 2 mm-long right fence (Z = 0.48) is worse, but it is far from the plug.
- **Outcome:** a hard yank cracks the left fence, or flexes it so the bead lets go. The board then slides about 1.44 mm, until the 470 µF overhang (y −51.81) hits the rim (−53.25). It is not catastrophic, but the cradle is one-shot.
- Insertion pushes +y and is fine: the top fences at x −58..−53 and −45..−40 bracket the plug line.

**Fix:**
1. **Add a bottom fence right under the plug:** `('x', -47.0, -41.0, cy0 - g_, -1)`. It is clear of:
   - the relay harness end (x ≤ −47.17 at z ≥ 7.77);
   - the (−38.5, −48.6) support pad (x ≥ −40.3);
   - the trough and bottom-fan wires, which are on top of the board at y ≥ −49.6.
2. **Thicken the two left-hand bottom fences** to `t_ = 2.0` (outer face y −52.13). Z rises to 4 mm³ and σ falls to about 11 MPa. Nothing runs over x −63..−41 at z ≤ 8.45. Keep the x 3..5 fence at 1.2: the IN pair passes above it.

**Confidence:** medium. The force is known, but the load sharing and the strength of a printed interlayer are estimates.

---

## S5 (Should-fix): in the right gutter, the SHT40 cable is squeezed between unfused R and the bus

**Evidence:** the gutter runs from the board edge (5.82) to the rib (14.75), 8.93 mm wide. My checker found two contacts the audit misses (their volume is under its threshold):

| Pair | Gap |
|---|---|
| **Field R** (L409: x 8.8, z 12, r 0.8; unfused 24 VAC) and **SHT40 (4)** (L460–461: riser (11, −21.5, 10) → (11, −17.5, 18), r 1.495) | **−0.087** (closest at y −20.5: axis gap 2.2 against radii 2.295) |
| **XL7015 IN pair** (bus, x 13.5, r 1.057) and the same SHT40 riser | **−0.051** (axis gap 2.5 against 2.552) |

**Why it matters:**
- The r5 board review spent a full round keeping 24 VAC at least 8 mm from logic on the board. The routing now puts the unfused R conductor in direct contact with the 3V3/I2C cable, which is soft silicone 26 AWG.
- It is insulated Class 2 wiring, so this is not a shock or creepage failure as such. It does mean a single insulation cut, for example under the cover-closing pressure at S1/S6, can put 24 VAC onto the I2C bus.

**Fix:**
- Keep the SHT40 low and rise late, at x 10.5: `(11, -40, 5) → (10.5, -26, 5) → (10.5, -18.5, 5) → (10.5, -17.5, 18)`.
  - At y −18, R is on its diagonal at x 7.93: gap 2.57 − 2.295 = +0.28.
  - IN pair: 13.5 − 10.5 = 3.0 against 2.55, so +0.45.
  - Fence R2 (x ≤ 7.17): clear.
- In the build notes, dress the SHT40 cable on the backplate floor with a dab of hot glue up to the bundle.
- L173–174 and L462 still say the cable runs "under the 470 µF's overhang". The cap is now at x −28..−8, so fix those comments.

**Confidence:** geometry high. Hazard low–medium.

---

## S6 (Should-fix): the controller-side bundle is 0.2 mm from the carrier in the cover's path

**Evidence:**
- L475: the waypoint (15.5, 2.5, 10.3) with r `r_bundle(10)` = 2.364 reaches x 17.86 and z 12.66.
- The carrier's left cradle (L747: s −2.75..12.75, 8.5 wide) spans x 17.25..32.75, y −8.25..0.25, and its plate bottom is at CAR_Z0 = 12.82.
- Then (14.5, 0, 13) → (14.5, −15, 15) runs at x up to 16.86, 0.39 from the cradle end. My checker gives a minimum of **0.21 mm**.
- The carrier arrives with the cover, moving −z. Any bulge of this 10-wire bundle toward +x is caught under the cradle's corner. Gravity (−y) doesn't help here, but bundles are never round.

**Fix:**
- At L433, set `xc = 13.8`, and change the waypoint to `(14.8, 2.5, 10.0)`. That gives about 0.9 mm or more of clearance.
- Clearances after the change:
  - the relay PCB edge at x 11 (bundle bottom z 10.6 > 7.6);
  - the XL harness below (top 7.5).
- Re-check against the field wires: the C wire passes under it at y −13.5..−19 with z 14.8 against a bundle bottom of about 12.6 at x 13.8. Both are offset in x.

**Confidence:** high on the geometry.

---

## S7 (Should-fix): the cradle is built on assumed board outline and thickness

**Evidence:**
- L52: `CTL = PB_X0 ∓ 5.79, PB_Y0 − 3.57 / + 9·2.54 + 3.57`. This assumes the 10 × 24 grid sits centred on a 70.00 × 30.00 board.
- Fences and bumps leave 0.15 per side (L614, L635).
- The bead underside meets the board top at exactly z 7.6 (L621), so a 1.6 mm board thickness is assumed.
- The USB window margin is ±1.25 in z and ±1.4 in x against a 12.35 × 6.5 plug.
- Cheap perfboards are often 1.5 or 1.2 thick, cut to ±0.3, with uncentred grids.
  - An oversize board (> 70.3 or > 30.3) won't drop in.
  - An offset grid moves the XIAO, and therefore the USB, relative to the window.
  - A thin board rattles under the beads and the press pins.

**Fix:**
- Add to "Measure before printing": the board outline, the edge to hole (0, 0) in x and y, the edge to hole (23, 9), and the thickness.
- Make CTL use measured margins (`PB_MARGIN_L, _R, _B, _T`) instead of symmetric 5.79 / 3.57.
- Set the bead z from the measured thickness: `CTL_TOP = 6.0 + t_board`.

**Confidence:** medium. Whether it bites depends on the actual board.

---

## Nice-to-have

**N1. Press pins are Ø2.4 × 17.25 mm free-standing (L550).**
- They print upward from the face. That is fine for supports, but each layer is a 4.5 mm² island, and PETG pins this slender lean or string.
- Make them r 1.5 with a 1 mm 45° root fillet.
- The (4.4, −21.4) pin root is 0.2 mm from the fuse-pocket edge (pocket x ≤ 3.0, L74/L587; pin x ≥ 3.2). Either make the pocket x1 2.6 or the pin x 4.8.

**N2. Field C passes 0.6 mm from the (4.4, −21.4) and (4.4, −48.4) pins' paths.** C runs at x 7.0, r 0.8, so x 6.2..7.8; the pins reach x ≤ 5.6. It is stiff 18 AWG, but a pin landing on a bowed C would crush it. Use x 7.4 for C. The fence top is 8.45, under C at 13.2, so that is fine.

**N3. The COM landing pushes on F2.** The 18 AWG COM in (13,8), x −25.37 (insulation r 0.8, so to x −24.57), overlaps the F2 holder base (x −24.8) by 0.23. Strip about 2 mm above the board and kink it to x −26.3 at once, as the riser already does. It is the same net as F2's output, so this is a fit issue, not a hazard.

**N4. Field Y1/G/O/W are modelled on top of each other** (audit 68.79 mm³ etc., L403). Model them as a 4-wire bundle, r = 0.8 × 1.15 × 2 = 1.84 at (y −6.5, z 13). I checked it:
- K terminals (y ≥ −4): 0.66 clear.
- COM jumpers at z 15.3 against a bundle top of 14.84: 0.46 clear.

It only matters so that later edits don't silently collide.

**N5. Stale text:**
- L173–174 and L462 ("under the 470 µF").
- L678: the grommet flange relief at (11.5, −23) is for a CTL screw that no longer exists. It is harmless, but delete it or note it.
- CONTROLLER_PARTS_TO_ADD.md still lists `CTL_HOLES` and "USB-C through the left wall".
- README still says "M3×4 heat-set inserts", "65–85" and "USB" placement.

**N6. The centre retaining pin's countersink is buried (L804–805).** With `reach = 7.0` the hole runs out through the guide column to 6.03 from the pin centre, but the countersink is cut at 3.95–4.30, inside the column. The 8.5 mm pin (L931) ends 1.78 mm inside the entry hole. You need a 1.5 mm push rod to seat it. Move the countersink to 5.7–6.05 for the centre switch.

**N7. IN+ against the TVS envelope: −0.04** (L448, box model). The real TVS is a Ø5.3 cylinder, so it probably clears. Check on the build that the IN+ wire passes the 1.9 mm TVS–cap gap without pushing the TVS.

**N8. Wall-screw access with everything fitted.**
- Left slot (x −48..−32, y −13.5): the driver path (r ~3) overlaps the cover bundle at y −19.6..−15 by 1.5 mm. It is pushable, and the S2 reroute to y −13 makes it worse.
- If you adopt S2, put the bundle at y −11 (−13.3..−8.7: the head plus driver needs y ≤ −16.5 or ≥ −10.5). Use z 19.5 and check the COM run at y −8 (−8.8..−7.2 at z 22.2+), which is OK.
- The right screw sits under the hot-glued D-pad bundle (x 20..41, y −9). Only relevant for removing the unit from the wall.

**N9. The XIAO rocks on its 8.5 mm female headers under plug insertion** (5–20 N at 11 mm above the board). Optional: a cover-hung plastic stop (no metal, so it is allowed at the antenna end) touching the XIAO's antenna end at y −20.3, z 16–17.
- It must clear the S2 bundle reroute.
- It takes the insertion load off the header pins.

---

## What I specifically checked in the round-6 changes

| Change | Result |
|---|---|
| D-pad guide pins | Socket depth OK. Pressed pin tip 0.4 above the socket floor (17.85 against 17.45). Rest engagement 2.5. The socket floor is 0.88 above the centre retaining-pin hole top (16.57). **Cap wall 0.31: see S3.** |
| Full-height slits | They clear the posts and pillars. The centre slit cuts partway into the DC→(55.1, 10.1) bar, but the bar stays continuous on its −side. |
| Raised sockets | The column socket wall is 0.5 on the outer side, merged with the cradle wall inside. |
| Post rings | Support-free (same r 3.6 as the tube). |
| 45° pillar bases | The cone r 0.6→0.8 emerges from a 1.2 wall at 45°. Verified for all four arrows, including the tangential down key. |
| SHT40 at SHT_X0 40, channel z 6 | Clear of the bottom boss, the standoff and the rest posts. The grommet squeeze and seat overlaps are intentional. Exhaust and intake are unchanged. |
| USB cut | Overmold depth: the receptacle face is at y −42.67, 14.3 mm from the outer face, so the plug body sits about 13.8 mm inside. The lane is clear of the pins (≥ 1 mm to (−38.5, −48.9) with a 15 mm-wide body) and of all wires (z ≤ 12.8). **Fingers: B1. Retention: S4.** |
| Fuse access | Cover off. Nothing over F1 or F2. The COM riser is 0.67 from the F2 end and the cover bundle is 1.5 mm past F2's top edge. |
| XIAO removal | Straight up, unobstructed (the trough wires are under it by design). |
| 24 VAC to logic and antenna | On the board and in the case: ≥ 12.3 mm to the antenna zone (COM riser) and 14+ mm (relay terminals and field wires at y −6.5). **In the gutter: S5.** |
