# openthermo enclosure v3 rev 2: spec for the second triple-agent review

## Ground truth and reference files
- **Model source:** /home/claude/thermo/fusion_case_v3.py, a Fusion 360 API script. It is self-contained and defines all helpers. All units are mm. The origin is the case centre. z = 0 is the wall face (backplate rear) and z rises toward the user.
- **Previous review (rev 1):** /tmp/claude-0/-home-claude/1e482d43-ef7a-56a9-9400-4044ee84b419/scratchpad/review/v3_critic.md, v3_sympathizer.md, v3_synthesis.md. Most rev-1 rulings were applied. Verify they were and don't re-raise solved items unless they regressed.
- **Previous spec:** spec_v3.md in the same folder, for background.
- **Measured interference** in the current model, all intended:
  - key nub on lever, 0.2 preload, 1.22 mm3 each;
  - carrier crush ribs on the switch body, 1.08 mm3 each;
  - OLED crush ribs, 1.2 mm3;
  - cover wall slots squeezing the sensor-cable grommet, 0.2 per side, 8.75 mm3.
  Nothing else overlaps.

## System (final parts; the user's parts on hand or ordered)
- **Controller:** XIAO ESP32-C6, Matter over Thread.
- **Relays:** AEDIKO 4-channel optocoupled relay module, 75×55 mm. The Dupont header is desoldered and the wires are soldered flat.
- **Power:** R → F1 T1A → 1N4007 half-wave (GND = C) → 470 µF 63 V (10×20) with a 1.5KE51A TVS → XL7015 buck at 5.0 V.
  - Model envelope for the XL7015: about 49×26×12 mm, mounted under the OLED on rails.
  - R → F2 T1.6A → relay COMs.
  - Fuses are in uxcell 5×20 PCB clip holders on the controller perfboard.
- **Sensor:** SHT40 module, about 18×12 mm, standing in the chamber at the bottom right.
- **UI:** 0.96" SSD1306 I2C OLED, or possibly a 2.42" SSD1309 later, which would need a re-layout and is out of scope here. The D-pad uses 5 lever micro switches.
- **Wire:** 26 AWG silicone, through a JST-PH 9-pin harness from the cover to the board.

## Changes since rev 1 (please scrutinise these)
1. **Wall mount.** Two #8 countersunk horizontal slots at x ±37.5, y -13.5, each ±10 long, for the Braeburn screw line (screws about 75 mm apart). The wire window is x ±17, y -19..-8, on a bare drywall hole with no box.
2. **Switch geometry (measured).** Body 12.8×5.8×6.0. The lever is 13.1 long and 3.5 wide, with its pivot inset 1.3 mm from the body end. The free tip is 3.75 above the body top. The switch clicks after about 1 mm at the tip.
   - Switches are lowered so the lever meets the key nub at ZF-1.8, with 0.2 preload.
   - Key travel to the stop legs is 1.2 mm, landing on the cradle wall tops (CRADLE_TOP).
3. **Switch retention.**
   - Crush ribs: r0.5, 0.1 bite, two per side and one on the end wall.
   - A 1.75 mm filament retaining pin passes through both cradle walls and the switch's own mounting hole (Ø2.0 / 2.1×2.0 slot, centre 1.5 above the body bottom, holes 6.5 apart). The entry wall hole is r0.975; the far wall hole is r0.875 for a press fit.
   - Each pin path was checked clear for 15 mm out from the wall (PINS dict).
   - Snap lips are disabled (USE_LIPS False).
4. **Carrier.** It is now a solid convex-hull plate rather than an X. Floor terminal openings are 3.6×12.4 so pre-soldered joints pass through.
5. **TPU grommet in the wire window.**
   - A flange (2.5 wide, 1.2 thick) sits on the plate's inner face, notched around the screw posts at (11.5,-23) and (8,-1.75).
   - A tube passes through the plate.
   - A stepped flared skirt (0.7 thick) protrudes 1.2 mm past the rear face and folds into a 1.8-wide × 1.2-deep groove around the window on the wall face.
   - A 0.4 mm membrane on the inside carries an I-shaped slit for the cable.
   - The strain-relief zip bridge moved to x -22.5..-19.8.
6. **Sensor-cable grommet (TPU).**
   - A 5.8×5×13 block at x 14.6..20.4, y -42.5..-37.5, z 3..16, standing on the plate. It sits in a notch cut through the divider rib.
   - A Ø2.8 channel at z 8 runs along x, with a 0.4 slit from the channel to the top.
   - The cover's divider (VDIV) and skin (VSKIN) have a slot open at their free edge, 0.4 narrower than the block. The cover slides down and squeezes the block shut. The block has 45° lead-ins on its top edges.
   - The old putty notch was removed.
7. **Rear gasket recess removed.** The backplate rear is flat except the window, the skirt groove and the screw slots.
8. **Thermal (from rev 1).** The XL7015 moved under the OLED. The chamber has a cover-hung double skin with a 2 mm air gap, a split transfer port and side intakes. A firmware offset is keyed to the energised relays.

## Printing
- PETG on a Bambu A1 mini or Snapmaker U1 at 0.2 mm layers. The cover prints face-down, the backplate back-down, the keys top-down, and the carrier plate-down.
- TPU 95A for the two grommets, printed as separate parts.
- The user wants everything printable without supports.

## Open questions for reviewers
- Will the window grommet skirt actually fold into the groove, or will it hold the backplate off the wall? Is the membrane slit workable for a 6-conductor 18 AWG thermostat cable (about 6×4 mm)?
- Can the sensor-cable grommet and cover-slot scheme be assembled? Does it seal? Will the cover sliding over TPU with 0.2 per side squeeze bind or tear it?
- Are the filament pins insertable after the carrier is printed, given the pin paths, finger access and press fit? Do the switch terminal openings weaken the carrier?
- Key feel and print risk with the new switch height and 1.2 travel.
- Anything that regressed versus the rev-1 synthesis.
