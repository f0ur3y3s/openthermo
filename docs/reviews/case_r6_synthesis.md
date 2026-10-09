# openthermo enclosure, round 6: SYNTHESIS

**Inputs:** BRIEF.md, fusion_case_v3.py (ground truth, all 964 lines), case_r6_critic.md, case_r6_sympathizer.md, audit.txt, HARDWARE.md, CONTROLLER_PARTS_TO_ADD.md, README.md, dpad_module.py header, the critic's `geo.py` / `boxes.py`.

**Method:**
- I re-derived every disputed number from the script.
- I copied the critic's checker to `scratchpad/synth_work/` and extended it:
  - `synth.py` adds the COM jumpers, the OLED pair, the six pads, all fences, the window-grommet flange (south strip, worst case), the cover tongue and skins, the relay terminals and relays, the USB lane, and the 470 µF as a **cylinder** (r 5.1, axis y −46.81, z 12.7) instead of a box.
  - `props*.py` compare the candidate routes, `final.py` checks the combined chosen set, and `extra.py` checks the press pins against the fence beads.
- Gap = axis distance minus radii; negative = overlap.
- I did not run Fusion and did not modify any input file.

**Bottom line:**
- One blocker: the floating fingers under the USB cut.
- Five cable/route fixes. All are code-level, and every one has been re-checked against every wire, fence, pin, pad, the rim, the cover walls, the carrier and the grommets.
- One real interference that both reviewers missed: press pin 1 against the right-fence bead.
- One printed-key fix.
- One board measurement.
- Two reviewer fixes are **rejected as written**, because they create new clashes: the critic's SHT40 re-route, and the critic's fence thickening. A third, the sympathizer's IN-pair z 9.6, nearly touches the cap.

---

## 1. Verification of every finding

Legend: **C** = confirmed, **P** = partly (geometry right, but severity, fix or detail differs), **R** = rejected.

### Critic

| # | Claim | Verdict | My check |
|---|---|---|---|
| B1 | USB cut plus intake slots −56/−51/−46/−41 leave three islands at z 14.2 | **C** | L574–575: slots cx ± 1, z 7..20. L578: cut x −55.73..−40.73, z 14.2..23.2. The remaining fingers are x −55..−52, −50..−47 and −45..−42, joined only at z 7. In face-down print, z 14.2 is printed before z 7, so they are true islands. Audit L73–75 lists exactly these three 6.0 mm² faces. |
| S1 | XL IN pair (y −53.4, z 9.3, r 1.057) sits 0.24 above the rim, 0.39 from its outer edge, 0.54 from the cover wall | **C** (geometry), **P** (detail) | Checker: rim 0.243, cover wall 0.543. **Error in the critic's cap number:** "0.63 to the 470 µF" is the edge gap at equal z. The true radial gap of the critic's y −52.3 route to the can (r 5.1) is **0.300**. Still clear. |
| S2 | Cover bundle 1.05 from the XIAO antenna end; drop 0.21 from the PCB corner; branch 1.24 from the PCB edge; sag toward pin (−32, −21.5) | **C** (geometry), **P** (fix) | Bundle edge −19.62 against the PCB end −20.67 gives 1.05. CoverB–XIAO_PCB 0.208. The pin edge −20.3 is 0.68 from the bundle edge, so the sag risk is plausible. **Fix problems:** (a) moving only the bundle start to (10, −13, 19.5) disconnects it from the Controller-side and SHT40 ends (the checker shows a −0.255 "gap" instead of a join); (b) z 19.5 puts the bundle top 0.39 under the COM run (z 22.2) for no benefit; (c) the critic's drop to (−62, −24, 15) is fine, but my route keeps the junction. |
| S3 | Centre-key guide hole leaves a 0.31 wall in the cap corner | **C** | Guides at DC + (∓3.203, ±3.203) (L125, L283–287). The corner arc centre is (±3, ±3) with r 1.5, so the hole centre sits 0.287 from it. Wall = 1.5 − 0.287 − 0.90 = **0.313**. Flat sides 0.397. Flange (half 5.3, r 2.3) 1.113. Moving the guides is impossible: the column must keep P ≥ 4.5 from the switch axis (the column inner edge is already 3.03 against the body's 2.9). Option (b) "flat-side 0.75" is mis-computed (actual ~0.91), but (b) is not chosen anyway. |
| S4 | USB pull-out goes into a 1.2 mm fence; σ ≈ 32 MPa across layers | **P** | Arithmetic confirmed: 81 % share, Z 1.44 mm³, M 46 N·mm, σ 32 MPa. **The critic's fix creates two new problems:** (1) t 2.0 raises the snap strain to 1.5·2.0·0.35/4.95² = **4.3 %**, at PETG yield; (2) the bottom-left fence's outer face at y −52.13 runs into the rim's inner corner arc (r 6.25 about (−59, −47), which is at y −51.65 at x −63.18), so its left 0.6 mm fuses to the rim and cannot flex. Replaced with a bead-less stop block (A4). |
| S5 | SHT40 cable touches unfused R (−0.087) and the IN pair (−0.051) | **C** (geometry), **P** (severity) | Checker reproduces −0.087 and −0.051. It is insulated Class 2 wire touching silicone 26 AWG, so this is hygiene rather than a hazard: should-fix, low. **The critic's fix is rejected:** with the rise at y −18.5..−17.5 at x 10.5, the riser passes R's corner at (8.8, −19, 12) at **−0.305**, worse than now. Its z 5 run to y −18.5 also cuts the window-grommet flange (z ≤ 4.2, y ≥ −21.5) by **−0.695**. Replaced (A5). |
| S6 | Controller-side bundle 0.21 from the carrier's left cradle end | **C**, **P** (fix) | Checker 0.211 at the (14.5, 0, 13) bend, against cradle end x 17.25, z ≥ 12.82. It is beside the cradle rather than under it. **Changing `xc` (L433) as proposed also moves the OLED (4) route (L471–472) and the XL OUT pair (L450),** because they share `xc`. Its waypoint z 10.0 also makes the D-pad (6) crossing −0.044. Use literals and keep z 10.3 (A6). |
| S7 | The cradle assumes a centred 70 × 30 grid and a 1.6 board | **C** | L52 and L72. The fences and bumps leave 0.15 per side, and the bead underside sits at exactly CTL_TOP. Nothing is measured yet. |
| N1 | Press pins Ø2.4 × 17.25 are slender; pin 3 is 0.2 from the fuse pocket | **C** (geometry), **P** (fix) | Pin 3 x 3.2 against pocket x1 3.0 gives 0.2. Do **not** grow pin 3 to r 1.5 (it would cut into the pocket), and do not move it to x 4.8 (it would then hit field C at x 6.2). Optional only. |
| N2 | Field C 0.6 from pins 1 and 3 | **C** | Checker 0.600. Stiff 18 AWG; no change needed. C at x 7.4 is checked and clean if the user wants it. |
| N3 | COM insulation overlaps the F2 base by 0.23 | **C** | Checker −0.230. Build note: strip and kink. |
| N4 | Y1/G/O/W modelled coincident | **C** | L403 `wy - i*0.0`. Model hygiene only. |
| N5 | Stale comments and docs | **C** | L173–174, L462, L678; CONTROLLER_PARTS_TO_ADD.md L21–22, 58, 69; README L16–17. |
| N6 | Centre retaining-pin countersink is buried; the pin end sits recessed | **P** | Confirmed: the reach-7 hole notches the guide column, and the 8.5 pin ends at lateral 4.25, about 1.6 inside the notch. Trivial: push it home with a filament offcut. Optional countersink move. |
| N7 | IN+ against the TVS box −0.04 | **C** as a box artefact | The real TVS is Ø5.3 about board (25.4, 2.54). The IN+ path clears the cylinder by about 1.0. |
| N8 | Wall-screw driver path blocked by the cover bundle | **C**; its y −11 suggestion **R** | Any bundle y in the allowed band (−15..−11.6, set by the antenna and the COM wire) blocks the driver over the slot (y −16.5..−10.5). **y −11 is 0.1 from the COM descent** at (−51, −8), z 17..23, which the critic missed. Accept: the screws matter only for removal, and the bundle is pushable. |
| N9 | XIAO rocks on its headers under plug insertion | **C** (plausible) | Optional cover-hung stop, numbers in §3. |
| "Checked" | 0 real audit interferences; 24 VAC ≥ 12.3 mm from the antenna | **C** | There are 65 pair lines in the audit (the critic said 74, the sympathizer 64). Both counts are slightly off; the conclusion is unaffected. |

### Sympathizer

| # | Claim | Verdict | My check |
|---|---|---|---|
| B1 | Same as critic B1; skip threshold 9.0 | **C**; threshold **P** | Both 9.0 and 10.5 drop exactly −56/−51/−46/−41 and keep −61 (12.77) and −36 (12.23), leaving 4.27 and 3.73 webs. **I choose 10.5** (cut half-width 7.5 + slot half-width 1 + 2 mm minimum web). It stays correct if `USB_C` moves after the board is measured (S7). 9.0 would allow a 0.5 mm sliver. |
| S1 | Guide-hole 0.31 wall; `CK_HOLE_TOP = ZF`, cut 5.65 | **C** | CK_PIN_CUT = 25 − 23.35 + 4 = 5.65 (L129 recomputes it). The flange wall is 1.11. The ceiling becomes a Ø1.8 bridge in a face-up print, which is trivial. "Check the slicer first" is moot: 0.31 is below one 0.4 bead. |
| S2 | Measure the board outline and grid offset | **C** | Same as critic S7. |
| S3 | IN pair to y −52.2 **and** z 9.6, because y −52.2 at z 9.3 would cross the bottom-right fence | **P** | **The fence argument is a straight-line error:** the tube meets the fence box at its corner. At z 9.3 the true gaps are y −52.3 **+0.233** and y −52.2 **+0.159**, so there is no crossing. **Raising to z 9.6 brings the pair to 0.061 from the real 470 µF can** (the sympathizer claimed about 0.3). Rejected in favour of y −52.4, z 9.3 (A2). |
| §1 | Cradle, pads, beads, USB lane, layering, D-pad, 24 VAC zoning, SHT40 | **C**, with one omission | Spot-checked: column 3.03 against 2.9; pin 1 0.61 to the terminal; pin 3 1.2 to F2; bead strain 2.6 %; corner gap 0.32. **Missed:** "every pin clears every board part" is false for pin 1 against the right-fence bead (§4, M1). |
| N1 | Delete the stale (11.5, −23) flange notch | **C** | Nothing printed or wired lives at z 3..4.2 there once the SHT40 follows A5 (0.305 over the restored flange). Pad (4, −24) top edge −22.2 against the flange −21.5 gives 0.7. Fence R2 ends at y −22.98. |
| N2 | Cover bundle to y −15, branch x −35.0 | **P** | Valid but weaker. The bundle edge is 3.35 from the antenna (the docs ask for 5–10), and the branch is 0.505 from pin (−32, −21.5). I choose y −13 and branch x −35.5, giving 5.35 and 1.0 (A3). |
| N3 | A fuse pull can lift the board | **C** (plausible) | Build note. |
| N4 | Snap-groove 1.25 ledge | **C**, no action | Accepted at round 3 on the fit ring. |
| N5 | Docs and comment nits, including the dead `zt_` at L614 | **C** | L614 is overwritten at L615. |
| §3 | Audit classification | **C** | No real interferences among the listed pairs. |

---

## 2. Disagreements resolved, and the chosen fixes

### 2.1 XL7015 IN pair: **y −52.4, z 9.3**

| Route | Rim corner | Bottom-right fence (corner) | 470 µF can (r 5.1) | Can at the max measured Ø10.5 | Outer edge vs rim inner face −53.25 |
|---|---|---|---|---|---|
| now (−53.4, 9.3) | 0.243 | 1.18 | 1.26 | — | 1.21 over the rim, 0.39 from its outer edge |
| critic (−52.3, 9.3) | 0.553 | 0.233 | 0.300 | 0.23 | −53.36 |
| sympathizer (−52.2, 9.6) | >0.6 | 0.385 | **0.061** | ≈0 | −53.26 |
| **chosen (−52.4, 9.3)** | **0.496** | **0.309** | **0.386** | **0.32** | −53.46 (1.39 inside the rim's outer edge) |

- **Chosen route checked against:**
  - field R and C at y −53.2 (z 12 and 14) and their risers into the terminal: > 0.7;
  - the R-C terminal envelope: 0.93;
  - press pins (4.4, −48.x) and (−38.5, −48.9): ≥ 1.1;
  - the VDIV tongue x 15.7: 1.14;
  - the rib: 0.193, unchanged;
  - the SHT40 (after A5): 0.45;
  - the cover's inner wall: > 2.
- **IN−** rises at x −35.53 (1.1 from pin 0). **IN+** keeps its x −29.4 thread between the TVS and the can.
- The pair has to clear both the fence and the can, so the margins are about 0.3 by construction. That is acceptable for flexible 26 AWG.

### 2.2 USB-vent skip threshold: **10.5**, as reasoned above.

### 2.3 Cover bundle and branch: **y −13, z 18.1, with a jog from the existing junction; drop at x −60; branch at x −35.5**

- **Allowed y band:**
  - The COM descent at (−51, −8), z 17..23 (r 0.8) limits the bundle centre to y ≤ −11.6.
  - The antenna 5 mm rule needs the bundle's south edge (y − 2.316) at least 5 from the XIAO end at −20.67, so y ≥ −13.35. Allowing for the real Ø5.8 bundle, y −13 is the practical lower limit.
  - That leaves a band of −13.35..−11.6. **y −13** sits at the antenna end of the band and keeps 1.9 to the COM descent.
- **z 18.1 is kept:**
  - bottom 15.8, over field C's top of 14.8;
  - top 20.4, which is 1.8 under the COM run at z 22.2.
- **Chosen route clearances:**
  - XIAO PCB and its z 16.1..20.8 envelope: 0.94;
  - header col 1: 1.6;
  - relay harness: > 0.6. A drop at (−60, −19.6, **13.5**) hit it (−0.14), so the drop ends at z 15.0. I also rejected (−59.5, −19.6, 15): 0.51 to the XIAO.
  - COM: 1.78;
  - pin 3: 2.6;
  - field C: 0.43 with a real Ø5.8 bundle;
  - relay terminals: y ≥ −4 against a bundle edge of −10.7;
  - F2: 4.3.
- **Branch at x −35.5:**
  - XIAO edge 3.0 (envelope);
  - pin (−32, −21.5) 1.0;
  - the 1 k under it 0.505, unchanged.
- **Antenna:** a 5.35 mm gap on the top side and 5.1 on the right side. The column-0 fan wires down the left side (1.26 from the PCB edge) are fixed by the round-5 pin map and cannot be fixed in the case. Hence the RSSI check (U2).

### 2.4 SHT40 riser (critic S5): **rise north of R's corner, and stay above the flange**

- **Route:** `(11, −40, 5) → (10.5, −26, 5) → (10.5, −22.5, 6.0) → (10.5, −17.0, 6.0) → (10.5, −16.0, 17.5) → (10.0, −17.3, 18.1)`.
- **Clearances:**
  - R: **0.639** (was −0.087);
  - IN pair: **0.450** (was −0.051);
  - grommet flange (z ≤ 4.2, with the notch removed per N1): 0.305;
  - fence R2: > 1.8.
- It ends in the bundle junction, as before.

### 2.5 Controller-side bundle (critic S6)

- **Route:** `(25, 2.5, 10.3) → (14.8, 2.5, 10.3) → (13.8, 0, 13) → (13.8, −15, 15) → (10, −17.3, 18.1)`, with the XL OUT pair ending at (13.8, −15, 15).
- **Clearances:**
  - carrier left end: **0.887** (was 0.211);
  - D-pad (6) crossing: 0.106, unchanged;
  - XL harness: 0.515;
  - relay PCB, K4 terminal and the (8, −1.75) boss: all clear (bundle bottom ≥ 7.9 at x ≥ 11.4).
- `xc` stays 14.5, so the OLED (4) route does not move.

### 2.6 USB retention (critic S4): **bead-less stop block fused to the rim, under the plug**

- **Block:** `tbox(-52.0, -44.0, -hy + RIM_T - 0.2, cy0 - g_, PLATE - 0.5, RIM_H)`, which is x −52..−44 (centred on the plug line −48.23), y −53.45..−50.13 (face 0.15 off the board edge, embedded 0.2 into the rim), z 2.5..8.0.
- **Strength:** no bead, so nothing has to flex. Z ≈ 6 × 3.3² / 6 ≈ 11 mm³, so σ ≈ 5 MPa even if it takes the whole 15 N.
- **Clearances:**
  - relay harness: 0.676;
  - bottom-fan envelope: 0.53;
  - pin 0: 4.3;
  - USB lane: 7.2 (block top 8.0 against the lane at z 15.2);
  - pads, and the left fence: 5.2;
  - IN−: 4.8.
- Printing: it is a plain block on the plate, so no overhang.
- The existing fences stay at t 1.2.

### 2.7 Centre-key guide hole: **option (a), `CK_HOLE_TOP = ZF`**

- No new part, so the Amazon Prime and wire constraints are untouched.
- Wall 1.11. Pin cut 5.65, which leaves 1.65 in the key plus a drop of CA. The guiding engagement (2.5 at rest) is unchanged.
- The pin never takes axial load: the pressed tip stays 0.4 above the socket floor.
- The critic's steel-wire option (b) is a fallback only if a CA-glued pin works loose.

---

## 3. Final action list

### Blocker

**A1. B1: floating fingers under the USB cut.**
- L574: wrap the bottom-intake loop:

```python
for cx in list(range(-56, 11, 5)) + list(range(21, 51, 4)):           # intake, bottom
    if abs(cx - USB_C[0]) < 7.5 + 1.0 + 2.0:                            # keep >= 2 mm web beside the USB cut
        continue                                                        # (drops -56, -51, -46, -41)
    cuts.append(trrect(cx - 1, cx + 1, -iy - 3, -iy + 1, 7.0, 20.0, 0.9))
```
- Result: the USB ceiling becomes a single 15 mm bridge, which is allowed.
- Re-run the audit and confirm the three `z 14.2` OVERHANG lines are gone.

### Should-fix

**A2. XL IN pair off the pinch line.**
- **Route:** in L444, L446 and L448, change every `-53.4` to `-52.4` (five occurrences). Keep z 9.3.
- **Model:** at L903–904, model the 470 µF as a cylinder so that the audit does not flag a false box overlap:

```python
if nm.startswith('470uF'):
    r.append((nm, tcyl((PB_X0 + 29.94, PB_Y0 - 0.4, CTL_TOP + 5.1), (PB_X0 + 49.94, PB_Y0 - 0.4, CTL_TOP + 5.1), 5.1))); continue
```
- **Build note:** dress the pair against the rim's inner face with Kapton or a dab of hot glue, and look along the bottom rim before closing.

**A3. Cover bundle and branch away from the antenna and pin (−32, −21.5).**
- L480: `wire_path([(10.0, -17.3, 18.1), (4.0, -13.0, 18.1), (-58.5, -13.0, 18.1), (-60.0, -19.6, 15.0)], r_bundle(15) * 0.8)`.
- L482: `[(-35.5, -13.0, 18.1), (-35.5, -22.0, 13.0), (-35.5, -28.0, 12.0)]`.
- Update the L477–479 comment to say "high along y −13, 5 mm off the XIAO's antenna end".
- **Build note:** lace the bundle with 2–3 small ties into a stiff loom so that it holds y −13 when the case is vertical.

**A4. USB pull-out stop.**
- After the fence loop (after L633), add:

```python
joins.append(tbox(-52.0, -44.0, -hy + RIM_T - 0.2, cy0 - g_, PLATE - 0.5, RIM_H))   # bead-less pull-out stop under the USB plug
```
- Do **not** thicken the existing fences.

**A5. SHT40 cable clear of R and the bus.**
- L460–461: after `(SG_X[0] - 0.2, sy, SG_CH_Z)`, use `(11.0, sy, 5.0), (10.5, -26.0, 5.0), (10.5, -22.5, 6.0), (10.5, -17.0, 6.0), (10.5, -16.0, 17.5), (10.0, -17.3, 18.1)`.
- Fix the comments at L173–174 and L462: the cable runs low up the right gutter and rises north of R's bend. The "under the 470 µF" wording is stale.

**A6. Controller-side bundle off the carrier.**
- L475: `... (25.0, 2.5, 10.3), (14.8, 2.5, 10.3), (13.8, 0.0, 13.0), (13.8, -15.0, 15.0), (10.0, -17.3, 18.1)`.
- L450: XL OUT pair end `(13.8, -15.0, 15.0)`.
- Leave `xc` (L433) at 14.5.

**A7 (new). Press pin 1 interferes with the right-fence-1 bead by 0.13 mm (§4, M1).**
- L83: change `PB_PRESS[1]` to `(4.4, -48.2)`.
- L618: change the first right fence to `('y', cy0 + 3.0, cy0 + 6.0, cx1 + g_, 1)`, which is y −46.98..−43.98.
- Results:
  - pin 1 to the right bead 0.42;
  - pin 1 to the bottom-right bead 0.23 (was 0.03);
  - pin 1 to the terminal 0.61;
  - field C 0.60;
  - the pin still lands on pad (4.4, −48.4).

**A8. Centre-key guide hole (critic S3 = sympathizer S1).**
- L128: `CK_HOLE_TOP = ZF`. This makes the pin cut 5.65 mm automatically.
- Docs: HARDWARE.md L183 and the dpad_module.py docstring L9: "7.9 mm" → "5.7 mm (cut 5.65), CA-glued in the key".
- dpad_module.py exec's this script, so the change carries over.

**A9. Board-dependent cradle (critic S7 = sympathizer S2): needs the user's measurement (U1).**
- Code: next to L49, add `PB_T = 1.6` and `PB_M = (5.79, 5.79, 3.57, 3.57)  # measured L, R, B, T: board edge to hole centre`.
- L52: `CTL = (PB_X0 - PB_M[0], PB_X0 + 23 * PB_PITCH + PB_M[1], PB_Y0 - PB_M[2], PB_Y0 + 9 * PB_PITCH + PB_M[3])`.
- L72: `CTL_TOP = 6.0 + PB_T`.
- L613 and L885: `CTL_TOP - 1.6` → `CTL_TOP - PB_T`.

### Nice-to-have

- **A10.** L678: `for cx, cy in ((8.0, -1.75),):`. This removes the stale (11.5, −23) flange notch. It is compatible with A5 (0.305).
- **A11.** Model Y1/G/O/W as one 4-wire lane (r 1.84 at y −6.5, z 13) or offset them, so that later edits don't hide collisions (L403).
- **A12.** L614: delete the dead `zt_ = CTL_TOP + 0.8`.
- **A13.** Optional: model the TVS as `tcyl((PB_X0 + 25.4, PB_Y0 + 2.54, CTL_TOP + 1.0), (PB_X0 + 25.4, PB_Y0 + 2.54, CTL_TOP + 10.5), 2.65)` to remove the −0.04 box artefact (N7).
- **A14.** Optional: in L804–805, for the centre switch only, put the countersink at 5.68–6.03 instead of 3.95–4.30. Otherwise, seat the pin with a filament offcut (N6).
- **A15.** Optional, only if pins 0–2 print wobbly: add a 45° root cone r 2.2 → 1.2 at the face. **Not on pin 3,** which is 0.2 from the fuse pocket (N1).
- **A16.** Optional, only if the XIAO visibly rocks under plug insertion: a cover-hung stop `tbox(-52, -44, -20.52, -18.5, 15.6, ZF + 0.3)`, which is 0.15 off the PCB end. It is 3.2 from the A3 bundle, 4 from the drop, and far from the branch. It is plastic only, but it touches the antenna end, so check RSSI with and without it (N9).
- **A17.** Docs:
  - CONTROLLER_PARTS_TO_ADD.md L21–22, 58 and 69 (`CTL_HOLES`, "USB-C through the left wall");
  - README L16–17 (heat-set inserts, 65–85, horizontal slots).

### Build-order notes (no code)

- Land the 18 AWG COM in (13,8) stripped about 2 mm above the board and kinked to x −26.3 at once, clear of the F2 base (N3).
- Hold the board down beside the holder, or use a fuse puller, when swapping a fuse (sympathizer N3).
- Unplug the USB before lifting the cover.
- The wall screws sit under the cover bundle. That only matters for removal: push the bundle aside (N8).

### Needs the user's measurement or decision

- **U1.** Measure the perfboard: length × width, the edge to hole (0,0) in x and y, the edge to hole (23,9), and the thickness. Put the values into `PB_M` and `PB_T` (A9). Sand the board if it is > 70.25 × 30.25.
- **U2.** After assembly, log Thread RSSI with the cover on, before the final install. If it is poor, use the U.FL/FPC fallback (GPIO14). The column-0 fan wires beside the antenna are fixed by the board pin map.
- **U3.** The 470 µF diameter is already on the build-step-1 list. Up to Ø10.5, the A2 route keeps ≥ 0.32. If the can is larger, tell me and the IN pair moves to y −52.6.
- **U4.** Decide on A16 (XIAO stop) only after a test insertion.

---

## 4. Missed by both reviewers

1. **M1. Press pin 1 (4.4, −48.4, r 1.2, z 7.75 up) cuts the bead of the first right fence by 0.13 mm.**
   - The bead's inner apex is a line at x 5.47, z 7.95, y −48.98..−45.98; the pin reaches x 5.6 at y −48.4.
   - The overlap volume is far below the audit's threshold, so the audit doesn't list Cover × Backplate.
   - The same pin is also only 0.03 from the bottom-right fence's bead apex (y −49.63).
   - **Effect:** when the cover goes home, the pin cams that fence outward, preloading the board and the bead, and the cover may not seat flat.
   - Fixed by A7.
2. **The critic's S5 route is worse than the current one** (R −0.305) **and cuts the window-grommet flange** (−0.695), because it ignored R's corner at (8.8, −19) and the flange at z ≤ 4.2.
3. **The critic's S4 thickening** reaches 4.3 % snap strain and fuses the bottom-left fence's end into the rim's corner arc.
4. **The critic's N8 "y −11"** is 0.1 from the COM descent at x −51. **The critic's S6 `xc` edit** also moves the OLED (4) and XL OUT routes.
5. **The sympathizer's S3 fence clash does not exist** (it is a corner gap, +0.16..0.23), and its z 9.6 nearly touches the 470 µF can (0.061).
6. **The IN pair grazes the TPU sensor grommet (0.043) and the rib (0.193)** where it descends the gutter at x 13.5. This is harmless, because TPU and plastic against insulation is fine. Note only.
7. **The `XL7015 IN- (C) to (9,0)` route stops at y −50.6, off the board,** and does not reach hole (9,0) at y −46.41. That hole sits inside the TVS **box** but 0.94 outside the real Ø5.3 TVS body. Reference only: drop the IN− lead straight into (9,0) beside the TVS. A13 removes the box artefact.

After the edits, re-run the build and the audit, and expect:
- no `z 14.2` OVERHANG;
- no new Cover × Backplate pair;
- no IN pair × 470 µF pair.
