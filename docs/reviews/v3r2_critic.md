# v3 rev 2 critic review

Ground truth: fusion_case_v3.py. All values in mm. Derived values:
- SW_Z1 = 25 − 1.8 − 3.75·10.1/13.1 = 20.309
- SW_Z0 = CAR_Z1 = 14.309
- CAR_Z0 = 13.109
- CRADLE_TOP = 20.959
- STOP_Z = 22.159
- NUB_Z0 = 23.0
- FL_Z0 = 24.2

## BLOCKER

### B1. Nub/lever preload is 0.63, not 0.2. The switch sits about 70% of the way to its click at rest (L52, L57–61, L414–416, L425)

**The geometry.** The lever slopes at 3.75/13.1, which is 16° (0.286 mm per mm), and rises toward the tip (s < 0). The nub is a flat-bottomed Ø3.0 cylinder (r1.5) centred on s = 0. LEV_AT_NUB (L58) is evaluated at the nub centre. On an inclined lever, however, a flat disc touches at its tip-side edge, s = −1.5.
- Lever z at s = −1.5 is 20.309 + 3.75·11.6/13.1 = **23.629**.
- NUB_Z0 is 23.0, so the real preload is **0.629**.

**Why the interference check missed it.** The reference lever is a stack of 0.3-thick slabs (L563). That caps the overlap depth. Integrating the stepped lever with the 0.3 cap gives 1.217 mm³, which is exactly the "1.22 mm³ intended" figure. Without the cap the overlap is 1.56 mm³. The check measured the slab thickness, not the preload.

**Consequences.** The contact arm is 11.6, so a 1.0 click at the tip equals 0.885 at the contact point.

| Tip click | Key travel to click (as modelled, contact at s = 0) | Key travel to click (real, contact at s = −1.5) | Tip deflection at stop (s = 0 / s = −1.5) |
|---|---|---|---|
| 0.8 | 0.42 | **0.08** | 1.82 / 2.07 |
| 1.0 | 0.57 | **0.26** | 1.82 / 2.07 |
| 1.3 | 0.80 | 0.52 | 1.82 / 2.07 |

The L53 comment says the click comes at 0.6–0.8 of travel. In reality it comes at about 0.26. That is a hair trigger. With an ordinary +0.25 stack-up (body seating, lever free-height scatter, cover face flatness), the switches rest *actuated*, which reads as a stuck key. It affects all 5 keys.

**Fix.**
1. Add `NUB_R = 1.0` and use it for the nub cylinders at L416 and L425.
2. Set `SW_Z1 = ZF - 1.8 - LEV_LIFT*(LEV_PIVOT + NUB_R)/LEV_L`, which gives 20.022. CRADLE_TOP and STOP_Z follow automatically.
3. Optionally, make the nub a 3.5 × 1.0 bar across the lever, with an r0.5 rounded bottom, so the contact s is defined.

Results:
- Preload 0.2 at s = −1.0.
- Click at about 0.65 of key travel.
- Tip deflection at the stop about 1.65, which is less than now.
- Make the reference lever slabs ≥ 1.0 thick (L563) so the interference check reports the true depth.

## MAJOR

### M1. Regression: the strain-relief zip tunnel is plugged by the left screw boss (L347–351)
- The bridge is x −22.5..−19.8. Its tunnel runs along x at z 2.4..4.8.
- The left slot boss is x −53.5..**−21.5**, z 2.5..4.5. It is in the same `joins` union, so it refills the tunnel for x −22.5..−21.5.
- That leaves a 0.3 slit (z 4.5..4.8) at the tunnel entrance, and a zip tie (about 1 × 2.5) cannot pass. Rev 1 had the bridge at −21..−18, so there was no overlap.
- The geometric overlap check could not see this, because it happens inside one body.

**Fix.** Put the tunnel on the boss top:
- `br = tbox(-22.5, -19.8, -18.5, -8.5, PLATE-0.5, PLATE+MNT_BOSS+3.8)`
- tunnel `tbox(-22.6, -19.7, -16.25, -10.75, PLATE+MNT_BOSS-0.01, PLATE+MNT_BOSS+1.8)`

Clearances: the bridge top at 8.3 clears the MOD PCB (y ≥ −7) by 1.5 in y, and the flange edge at x −19.5 by 0.3. Do not trim the boss instead: the #8 head reaches x −23.19.

### M2. The window-grommet skirt will most likely hold the backplate off the wall, and the stack only tolerates 0.2 (L79–80, L378–390, L368)
- **Skirt shape.** The skirt runs from the groove ceiling (z 1.2) to z −1.2, which is **2.4 long**. It is 0.7 thick, and its outer offset steps from 0.075 to 0.95.
- **Groove.** 1.8 wide × 1.2 deep.
- **Fold at the wall plane (best case).** The 1.2 protruding length lies flat from about offset 0.5 to 1.7. That leaves 0.1 to the groove's outer wall, before allowing for the bend radius (≥ t/2 = 0.35).
- **Fold at the stiff root (z 1.2).** The tip lands at offset about 2.4, beyond the groove. It is then pinched between the plate land and the wall, giving a standoff of about 0.7.
- **Corners.** Folding outward stretches the skirt from r 4.5 to r 5.7, a hoop strain of **27%**. TPU 95A will wrinkle there. A doubled 0.7 wrinkle is 1.4 thick, which is more than the 1.2 groove depth.

**Tolerance budget.** The backplate references the wall, and so does the cover rim (z 0). A backplate standoff Δ shifts the following:
- The bottom M3: cover hole r1.7 vs screw r1.5 gives **0.2** slack (L317, L363).
- The snap bead: 4.4..5.6 inside a 4.2..5.8 groove, so ±0.2.
- The VDIV tongue: 0.4 to the groove floor.
- The sensor-grommet top against the slot roof: 0.0.

So Δ > 0.2 already binds the cover screw.

**Fix.**
- Set `GRM_SKIRT_PROUD 0.6`, `GRM_SKIRT_T 0.5`, `GRM_SKIRT_FLARE 0.6`, and shorten the skirt so it starts at z 0.6 instead of the groove ceiling (z 1.2).
  - The folded flap then sits at offset about 0.4..1.0, leaving 0.8 of margin.
  - Corner hoop strain drops to about 13%.
- Keep the groove at 1.8. Widening it to 2.6 would break into the CTL insert hole at (11.5, −23): r2 reaches y −21, the groove edge is now at −20.8, and the hole floor is at z 1.0 while the groove ceiling is at 1.2.
- As a safety margin, elongate the cover's bottom screw hole in z by 0.6.

### M3. The membrane slit leaks permanently and won't grip the cable (L78, L392–396)
- **Printed openings.** The slit is printed open, 0.6 × 26, plus two 0.6 × 6 cross slits. That is about **23 mm²** open with no cable, roughly a Ø5.4 hole.
- **Flaps.** The cross slits turn the membrane into two 26 × 4 flaps hinged only on their long edges. A 6 × 4 cable folds them open along most of their length, leaving wedge gaps on both sides of the cable.
- **Fix.** Delete the slit and cross-slit cuts and print the 0.4 membrane solid. On install, knife-cut a single 7–8 mm slit, or a 6 × 6 "+", where the cable actually emerges.
- **Severity.** Foam in the drywall hole is still the primary seal, but as modelled this membrane seals nothing.

## MINOR

### m1. The sensor-cable grommet doesn't seal at the divider plane (L272, L84–87, L293, L366)
- **The divider doesn't reach the channel.** VDIV starts at z **10.4**, but the channel spans z 6.6..9.4. At the divider plane the cable and slit sit only inside the rib notch, which has **0.1 clearance per side** (notch y −42.6..−37.4, block −42.5..−37.5).
- **Leak area.**
  - Rib-notch gaps: 2 × 0.1 × 7.4 = 1.5 mm².
  - Unsqueezed slit at z 8..10.4: 0.4 × 2.4 ≈ 1 mm².
- **The skin isn't a seal either.** VSKIN does squeeze the channel zone, but it is not a seal: its free edge sits 0.5 above the plate along its whole 18 mm length.
- **The squeeze only closes the slit.** SG_SQUEEZE of 0.2 per side is 0.4 total, which equals SG_SLIT. The slit closes with **zero contact pressure**.
- **The cable props the slit open.** 4 × 26 AWG silicone (Ø1.3 each) has a circumscribed bundle of about 3.1, larger than the Ø2.8 channel, so it holds the slit open by about 0.3.
- **Severity.** The total leak is of the same order as the existing tongue-and-groove labyrinth, so this is minor.
- **Fix.**
  - Rib-notch cut at y `SG_Y[0]+0.1..SG_Y[1]-0.1`, which gives a 0.1 per side press over z 3..13 and also stops the block lifting out with the cover on removal. Today it would drag the sensor cable.
  - `SG_SQUEEZE 0.35`.
  - `SG_CH_D 3.2`.
  - Print the slit closed and knife-cut it.

### m2. The sensor grommet engages blind, before the rim guides the cover
- The VSKIN prong tips (z 3.5) meet the block top (z 16) when the cover is still **12.5** from home. The rim only engages at **8** (RIM_H).
- That leaves 4.5 of blind, hand-aligned travel with ±0.65 y capture (0.85 lead-in − 0.2 squeeze).
- **Fix.** Add 1.0 × 45° chamfers on the slot mouths of the VSKIN and VDIV prongs. Optionally lower the block top to 13, flush with the rib top.

### m3. The pin holes will print tight (L92, L479–480)
- Horizontal Ø1.95 and Ø1.75 holes at 0.2 layers sag at the roof. Expect about Ø1.8 in Z, so the *entry* hole also becomes a press fit. A filament pin then has to press through two walls blind, while hitting a Ø2.0 switch hole with ±0.125 z tolerance.
- **Fix.**
  - `PIN_ENTRY_R 1.05` with a 0.3 × 45° countersink on the entry face.
  - `PIN_FAR_R 0.9`, or ream it with a Ø1.8 drill.
- **Answering the open question.** The paths themselves are fine: on the bench the carrier is free on all sides, and I spot-checked the centre path (it clears the up cradle at y 5.6 by ≥ 2.3 in x). The pins are insertable once the holes are opened up.

### m4. The TPU print orientation must be specified
- **Window grommet.** Print it flange/membrane-down.
  - The membrane is then 2 bed layers.
  - The skirt flares at 22.6° from vertical.
  - The tube-to-skirt step is 0.125.
  - Printed skirt-down instead, the flange overhangs **2.55** with nothing under it.
- **Sensor block.** Print it base-down, and print the slit closed (see m1). A 0.4 TPU gap tends to be welded shut by stringing anyway.

### m5. Small leftovers
- The flange notch at (11.5, −23), r3.8, leaves only 0.15 of flange beside the tube along x ≈ 9–13. This is acceptable because the flange retains rather than seals.
- The right-key relief at L430 is now redundant: the down lever reaches about 20.6 against the flange at 23.0 at full travel. It is harmless.

## Verified OK (don't re-raise)
- **Ghost press from rev 1 is resolved.** The centre lever at R6.3 is at about 21.4 against a ring flange at 23.0 at full travel. The 2.0 diagonal gaps exceed the 1.75 lever half-width.
- **Stop legs.** All of them land centred on 1.2 walls: offset 3.65, and the centre legs at s 2.0, n ±3.656. Travel is 22.159 − 20.959 = 1.2.
- **Lever tip vs own flange.** Clear at rest (≥ 0.85) and at the stop (≥ 1.0).
- **Carrier hull.**
  - Against the bulk cap (y ≤ −20.2 vs hull −19.25): 0.95.
  - Against the XL7015 (y 21.5 vs 19.75): 1.75.
  - Spoke to the cap: clear.
  - The terminal slots leave ≥ 1.25 floor ledges inside the walls. The plate is not meaningfully weakened.
- **Mount.** The countersink land is 2.08. Flange to boss is 2.0. The groove-ceiling bridge (1.8) and the rail and zip tunnels are printable back-down. The cover slots are open at the bed side when printed face-down.
