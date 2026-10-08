# v3 enclosure: critic review

Source: /home/claude/thermo/fusion_case_v3.py (line numbers cited as Lnnn). All units are mm, with z=0 at the wall.

## VERIFY FIRST: the screenshot does not match the code
The front screenshot shows a raised (or deep-recessed) rounded-rectangle block with its own window. It sits upper-left of centre, over roughly x -40..+5 and y 15..55. No v3 code produces a front feature there. The cover cuts are L229-249, and the only front openings are the OLED window and the D-pad. `build()` deletes only `root.occurrences` (L193-194), so root-level bodies or sketches from an earlier run or v2 survive. Check the Browser and delete them. Then re-screenshot and confirm that the STL/3MF export holds only the 4 printed components.

## BLOCKER (conditional: single-gang box install)
**B1. The wire window is not over the gang-box opening.**
- The gang holes are at (41, ±41.65) (L60, L289-290), so a box mounted on them is centred at (41, 0).
- A standard single-gang device box opening is about 50 × 75, which covers x ≈ 16..66 and y ≈ -37..37.
- The wire window is x -17..17, y -19..-8 (L285), and its r4 corners take most of the 1 mm that overlaps x 16..17.
- Result: the wall wires come out of the box behind solid backplate.
- The bottom slotted hole (41, -41.65) is also inside the sensor chamber floor. Its unused slot length (±5.9 slot ends vs a ±3.75 head) opens about 2 × 2.15 × 3.8 to the box or wall cavity air.

Fix:
- Move the window into the UI column inside the opening: `trrect(22, 40, -29, -21, ...)`. Keep the foam ring around it at +4 (x 18..44, y -33..-17). It clears hrib (y ≥ -30.9 needs window y ≤ -31.5, which is OK at -29) and F2. The carrier screw head at (25, -20) is at z 11.5..13.5, well above the window wires at z ≤ 10.
- Wires then cross to the relay terminals through y -20..-7, which has no divider.

If the install is a bare drywall hole (the old Braeburn), this is not a blocker. In that case the 83.3 pitch is arbitrary, and the 11 mm window height is the main positioning constraint.

## MAJOR
**M1. Thermal: mode-dependent self-heating with weak isolation.**

Power budget:
- Coils draw 5 V × 70 mA = 0.35 W each, not 0.3 W total. Heat+Aux and Cool each energise 3 relays, which is 1.05 W.
- The XL7015 at about 34 V in, 5 V out, 0.3 A runs at about 78 % efficiency, so it loses about 0.4 W.
- The XIAO and its LDO add about 0.3 W.
- Total is about 0.6-0.7 W idle and about 1.8 W running.
- A swing of about 1.1 W tracks the call state. This is a positive feedback bias: the sensor reads warm while the system is cooling.

Coupling paths:
- (a) The cable notch L230 leaves about 4 × 3.5 = 14 mm² open between the chamber and the CTL bay. This is z 12..16 above the 12 mm backplate rib, and only 6.5 mm from the XL7015's right end (x 8).
- (b) The single 1.6 mm divider is 7.7 mm from the XL7015.
- (c) The screw slot, as in B1.

Fix:
- Close the notch after cabling, or replace it with a Ø3 hole and grommet.
- Make the chamber a double wall: keep the cover divider and add a full-height parallel backplate wall at x 19.5 and y -35.5, leaving a 2 mm air gap.
- Mirror the CTL layout: XL7015 at the far left (x -60..-16, y -51..-35). Put the XIAO and the 470 µF cap next to the divider. Moving the XIAO means the USB slot moves to the bottom wall.
- Add a firmware offset per energised relay, calibrated against a reference sensor.
- Better still, use latching relays or a small SSR/TRIAC board to remove the 1 W of coil heat.

**M2. F1 blocks the K1 terminal wire entry, and the holder type is unresolved.**
- The K1 terminal spans x -58.6..-43.4, y -7..1, and its wire mouths face -y (L362).
- The F1 box (L352) is at y -18..-10, z 3..13. That leaves 3 mm in front of the K1 mouths, where 18 AWG needs about 10 mm.
- The 22 × 8 × 10 envelope in the model is a PCB clip-with-cover holder, not a uxcell inline holder (a barrel of about Ø11-13 × 45-60 with flying leads) or a panel holder (needs a Ø12-13 hole and 30-45 depth).

Fix:
- Use PCB 5×20 holders with covers (about 22.5 × 8.5 × 11).
- Move F1 to the UI column at x 42..64, y -29.5..-21. This clears the down-switch pins (y -15, z ≥ 11.2), the post (57, -18) screw head (z ≥ 11.5), and hrib (-30.9).
- If you stay inline, lay the barrel in the y -20..-7 channel at x -40..+5 only, not in front of K1/K2. That requires B1's window move.

**M3. The Dupont header does not fit as modelled.**
- L344 puts the top at 24.1 vs ZF 25, leaving 0.9 mm and no allowance for the wire exit. A housing plus a 90° wire bend needs 5+ mm more.
- A right-angle header is impossible: there are only 7 mm from the PCB edge (y 48) to the wall (55), and the housing is 14 mm.

Fix: desolder the 6-pin header and solder wires flat (top about z 11). Keep the JD-VCC jumper, which is about z 19 and OK.

**M4. Key feel: rattle, no hard stop, possible ghost presses.**
- The nub-to-lever gap is +0.3 (NUB_Z0 23.5 vs LEV_TOP 23.2, L52/L55). Keys float 0.3 axially and 0.3 laterally, so they rattle.
- Nothing stops a key before the lever bottoms on the switch body (about 1.9 mm of nub travel), so overtravel goes into the switch.
- The up and right flanges (z 24.2) sit 1.0 above the centre-switch lever, which runs along their shared diagonal (L129, lever ±1.5). The right flange also overlaps the down lever's pivot end at r 13-16.

Fix:
- Set the nub gap to -0.2 (preload) so the lever spring seats the key.
- Add 2 stop legs (Ø1.6) under each ring flange at ±3.65 from the key axis, landing on the cradle wall tops (CRADLE_TOP 20.5). Set leg bottoms at z 21.7, which gives 1.2 mm total travel.
- Add a stop ring for the centre key in the same way.

**M5. The cover is tethered by about 9 wires with no connector.**
The OLED (4 wires) and 5 switches plus GND all run to the XIAO, so changing a fuse means dragging the cover around on a harness. Fix: add a 1 × 9 JST-PH (or a 4-pin and a 6-pin) on the CTL board, with a 60 mm slack loop.

## MINOR
1. **XL7015 height is unverified.** Common modules use 10 × 16 caps. The budget from the perfboard top is 25 - 7.6 - 0.5 = 16.9 mm, including any standoff. If the module is over 14 mm, seat it directly on the backplate (z 3) to gain 4.6 mm.
2. **The SHT40 does not match the rail.** The spec says 18 × 12, but the rail slot is 16 long (L274-275) and the model shows 13 × 10. Lengthen the slot to UX ± 9.7, and check that an 18 mm-tall orientation reaches only z 24.
3. **Elephant foot on key clearances.** The 0.3 radial and side clearances (L47-50) fall on first layers for both the cover (face-down) and the keys (cap-down). Use 0.45, plus a 0.4 × 45° chamfer on the outer edge of each hole.
4. **Ring-flange gaps are tight.** The gap between ring flanges is 0.2 (d = 0.1 each, L303). Raise d to 0.18 for a 0.35 gap.
5. **Keys are flush with the face** (the cap top is at DEPTH 27). Make them 1.0 proud for findability, with the cap at z 25..28.
6. **OLED retention is friction only** (0.15 crush ribs, L221-223), with nothing behind it. Add 4 Ø1.8 heat-stake pins at the module's corner holes, or a printed clip off the frame.
7. **Switches fall out during assembly.** Pockets have 0.15 clearance on every side, and the carrier is installed inverted. Use -0.05 crush ribs or a dot of CA.
8. **Insert sizes.** The relay posts have only 5.1 of hole depth (L281), so use M3 × 4 short inserts rather than 5.7. CTL Ø3.5 implies M2.5 inserts. The cover-screw boss Ø4 takes a horizontal M3 insert. That is 3 insert SKUs in total.
9. **Thin skin in the bottom chamber vent.** The slot at cy = -50 (L238-239) starts at x 66, but the inner corner wall is at x 65.93 when y = -51, which leaves a 0.07 mm skin. Start the cut at ix - 3.
10. **The backplate hangs from one vertical screw line at x = 41**, leaving the left 108 mm unsupported, and the countersink leaves a 0.87 mm land. Add a third Ø3.8 countersunk hole at (-52, -13.5) once F1 moves; it is reachable with the module installed. Use a 4 mm local plate thickness at the holes.
11. **No strain relief for the wall cable.** Add a zip-tie bridge of 3 × 4 posts at y -23 beside the window.
12. **The transfer port is a 26 mm bridge** when printed face-down (L231). It is OK in PETG, but 2 ribs splitting it into 3 × 8.7 is cleaner.
13. **Relay channel LEDs glow through the top vents** (x -58..8) onto the wall at night. Mask them with Kapton or paint.
14. **Spec inaccuracies.**
    - The D-pad webs are 1.6 (2 × d), not 0.8.
    - The x = 16.5 divider exists only below y -31.7, so the bays are not separated.
    - The "down keeps clear of XL7015" comment (L128) is stale from v2.
15. **Aesthetics.** The 138 × 114 face is about 1.4× an Ecobee's, and the left 2/3 is blank while all the UI is crammed into the right column. Consider a recessed 0.4 mm groove or a logo panel on the left, or centre the OLED over the full width with the D-pad below it.

## Verified OK (no action)
- XIAO USB slot alignment: centre y -36.25, z 18.75 matches the receptacle, and the 5.5 mm interior gap is clear for a 12 × 6.5 overmold.
- Relay tops at 23.1 leave 1.9 to the face.
- The divider-in-groove engagement is 1.6.
- The snap bead interference is 0.35 into a 0.8 groove.
- The cover screw (Ø3.4 into a Ø4 insert boss) is reachable from below.
- No switch body or cradle collides with another.
- The carrier posts clear the key flanges by 0.9.
- The front chamfer is a 45° face-down overhang.
- Top and bottom vents clear the inner corners.
