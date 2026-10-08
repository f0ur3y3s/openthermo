# Sympathizer report: openthermo v3 wiring, placement and enclosure (review round 3)

All coordinates were computed from the parameters in `/home/claude/thermo/fusion_case_v3.py` (the "code" below).
The frame is the script's: X right, Y up, Z out of the wall, wall at z = 0 and front face at z = 27.
Values derived from the parameters: cover inner half-extents ix = 67 and iy = 55; backplate rim outer face hx = 66.85 and inner face 65.25; ZF = 25.
For the D-pad: SW_Z1 = 20.02, SW_Z0 = 14.02, CAR_Z0 = 12.82, SFX_Z0 = 23.35 and STOP_Z = 21.85.

Verdict up front: the design is sound in its architecture. All the energy stays on the Class 2 24 VAC side, and the power chain is well chosen. The field side and the logic side are physically separated by the relay module itself, and the sensor sits upstream of every heat source.
There are two problems that are real and cheap to fix: driver access to one controller-board screw, and documentation drift. There is also one item worth a measurement before printing: the D-pad's travel to the click.

---

## 1. Electrical wiring and safety

### Sound choices
- **Everything in the box is Class 2.** There is no mains in the enclosure: the highest voltage is the ~39 V DC bus. That removes the usual creepage and enclosure-rating worries a skeptic might import from line-voltage practice.
- **Half-wave rectification with C as GND** (HARDWARE_r1 lines 20–28; CLAUDE.md line 77).
  - This is the right topology for an earth-referenced C. A bridge rectifier would put the board's ground at a diode drop from C on alternate half-cycles. USB to a PC would then be unsafe, and the relay module's DC− would no longer equal the contact-side reference.
- **The power chain is sized with margin.**
  - The bus is about 24–28 VAC × √2 − 0.7, which is about 33–39 V. A 30 VAC unloaded transformer gives about 42 V.
  - The 1.5KE51A has a 43.6 V standoff, so it does not conduct in normal service, and it breaks down at about 48–54 V. That is below the XL7015's 80 V limit.
  - The 470 µF cap is rated 63 V, which also covers the TVS clamp region for short transients.
  - Ripple: at a ~60 mA bus draw (≈1.5 W out at 5 V), ΔV = I/(f·C) ≈ 2.1 V p-p. That is irrelevant against a 5–80 V buck input range.
- **Fusing.**
  - F1 (T1A) protects the power stage. If the 1N4007 fails short and the cap sees AC, F1 is what clears the fault.
  - F2 (T1.6A, slow-blow) on the R→COM path is sized near a 40 VA transformer's 1.67 A. That is below the typical 3–5 A air-handler board fuse, so a thermostat-side short should blow F2 and not the air handler's fuse.
  - Slow-blow tolerates contactor inrush.
- **The 1N5819 between the 5 V rail and the XIAO 5V pin** stops USB from back-feeding the relay coils and the buck output during bench flashing. The ~0.3 V drop is harmless ahead of the XIAO's LDO.
- **Pin map.** I checked it against the hard rules in CLAUDE.md lines 74–76 and HARDWARE_r1 lines 51–69.
  - The relay pins are GPIO1, 2, 21 and 18. None of them is a strapping pin (C6 straps are GPIO4/5/8/9/15) or UART TX (GPIO16).
  - The 10 k pull-downs, combined with an H-trigger module, make "floating" read as "off" during the boot window. This is exactly the right pairing.
- **18 AWG is used only where the contact current flows** (F2→COM1, the COM links, R/C on the board). 26 AWG carries only logic and the ~60 mA buck input. That allocation is correct for the wire on hand.
- **Firmware defence in depth already exists** (CLAUDE.md lines 47–58): the independent `relays_guard`, the boot-armed minimum-off, and an all-off fault path. The wiring does not need to carry timing guarantees.

### Likely criticisms
| Criticism | Assessment |
|---|---|
| "Opto-isolation is defeated because DC− = C." | **Overstated.** Isolation is not a requirement: the whole system shares one Class 2 reference by design, and the opto merely buffers the GPIO. |
| "Relay 12-way terminal order is unconfirmed, so a field wire could land on NC. Y1 would then be ON whenever the relay is off, defeating min-off at boot." | **Real (safety-relevant) but already caught at bring-up.** Step 2 checks COM→NO continuity (HARDWARE_r1 line 109). **Smallest fix:** add an explicit bench check: *with the module unpowered, every field terminal must read open to COM*. Mark NO on the module with paint. |
| "Will 3.3 V GPIO drive a 5 V H-trigger opto module?" | **UNVERIFIED.** It usually works (about 2 mA into the opto LED), but bring-up step 2 says "from 5 V". **Smallest fix:** run step 2 from a 3.3 V source, or with the XIAO itself. |
| "D6 (right) is U0TXD: pressing it during boot shorts a driven-high TX to GND." | **Real but minor.** It is documented (HARDWARE_r1 line 62) and brief. **Smallest fix:** a 1 k series resistor in the D6 switch line; it costs nothing to the pull-up input. |
| "No RC snubber or MOV across the relay contacts switching 24 VAC coils." | **Mostly overstated.** SRD contacts are rated 10 A, and the loads are about 0.2–1.25 A of 24 VAC. Arcing wear is negligible at that level. An optional 47 Ω + 100 nF across the Y1 contact would help EMI on the D-pad's high-impedance inputs, but firmware debounce already covers it. |
| "2 × 18 AWG in one screw terminal at COM2/COM3." | **Overstated.** 5.08 mm blocks take two 18 AWG conductors. A twin ferrule is a nicety, not a requirement. |
| "Does the XIAO C6 already have a VBUS diode (double drop)?" | **UNVERIFIED.** It is harmless either way: 4.4–4.7 V is still above the LDO's dropout for 3.3 V at this current. |

---

## 2. Component placement and wire routing

### Sound choices
- **The field side and the logic side sit on opposite edges of the relay module.**
  - The 12-way output block faces the wire window: its front face is y = my0 + 0.5 = −4.0, and the window top is y = −8 (code lines 112, 365–373).
  - The 6-way input block is on the far edge, at y 37..45 (line 771).
  - The relay harness reaches it by going round the left side.
  - So the 24 VAC field conductors never cross the logic harness. That is good practice.
- **The field-wire path is short and direct.** The wires come up through the window at y = −13.5 (the wall hole, centred between the screws, MNT_Y). They rise to z 13 and turn at y −6.5, inside the 3.5 mm gap between the window and the module. Then they enter the terminals at their entry height, zt = 12.6 (lines 362, 368–373).
  - R and C drop the other way into the R-C terminal (x −10..0, y −29..−21), which sits right next to F1 and F2 (x −36..−9.2).
  - The zip-tie bridge (x −22.5..−19.8, y −18.5..−8.5, line 550) gives the field cable strain relief right at the window.
- **The relay harness has more room than the brief says.**
  - Bundle: r_bundle(6) = 1.83, so the tube spans x −65.03..−61.37 at z = 11.43.
  - That is above the backplate rim (RIM_H 8), so the real gap is cover inner face (−67) to module edge (−62) = 5 mm, not 3 mm.
  - The tube clears the input terminal (x −61.0) by 0.37 mm in the idealised model. A loose 26 AWG bundle will simply flatten.
- **The shared channel at x ≈ 14 stacks cleanly.**
  - The XL7015 cable (r 1.29) runs at z 6. The cover harness (r 2.36) runs at z 12 above it.
  - Both pass between the relay PCB edge (x 11) and the D-pad carrier's left edge (x 17.25, z ≥ 12.8).
  - They also clear the XL7015 board (x ≥ 16.5) by about 0.5 mm.
  - Under the carrier, z 3..~10 is open across the whole UI column (y −30..20). That is ample room to stow the inline JST-PH pigtail pairs, so the cover can come off without strain.
- **Vertical stack-up clears everywhere.**
  - Relay tops are at 6 + 1.6 + 15.5 = 23.1, which leaves 1.9 mm to ZF. HARDWARE_r1 line 37 says 1–2 mm, which is consistent.
  - XL7015 parts top out at 16.5, against the OLED frame bottom at 20 and the OLED back parts at 21.6. That leaves **3.5 mm of margin for the unmeasured XL height**: the board could be up to ~15.5 mm tall before anything touches.
  - The fuse holders with caps reach 24.6, against ZF 25 (0.4 mm). They are capped, so going cap-off only adds margin.
- **The USB-C opening lines up with the receptacle.** The cut is at y −42.75..−29.75 and z 15..22.5 (line 509). The shield or USB box is centred at y −36.25, z ≈ 17.7 (line 775).
- **The sensor chamber flows as a chimney.**
  - Room air enters at the bottom (x 21..49, z 7..20) and the right side (y −50..−38). It passes the SHT40, which lies flat at z 14–15.6, mid-stream.
  - It exits through the ribbed port (y −36.7..−30.5, z 16..21) into the UI column, past the XL7015 (y 26–42), and out of the top vents.
  - **Every heat source is downstream of the sensor.** The relays are in the left column, which has its own intake and exhaust.
- **The SHT40 cable path is a labyrinth, not a hole.**
  - The cover's divider tongue (z ≥ 10.4) drops into the backplate rib slot (z 10..12), giving 1.6 mm of engagement.
  - The cable crosses the double skin in a TPU block that the cover squeezes 0.15 per side (lines 485–486, 567). Its channel is 3.2 mm across, for 4 × 26 AWG silicone wire.

### Likely criticisms
| Criticism | Assessment |
|---|---|
| "The controller board's screw at (11.5, −49) is under the 470 µF cap." | **Real.** The cap is modelled as a cylinder on the axis x = 7, z = 12.6, r 5, spanning y −50.5..−29.5 (line 781). It overhangs that hole: at x 11.5 it occupies z 10.4..14.8. A screw head (top at ≈9.3) fits underneath, but **no driver can reach the screw** once the cap is soldered, and the cap is soldered before the board goes in. **Smallest fix:** move the cap axis to x ≈ 4 (body x −1..9). That still clears the R-C terminal (which ends at y −21) and the fuse holders (x ≤ −9.2). Alternatively, leave that hole unscrewed: three M3 screws hold a 74 × 32 perfboard fine. |
| "The perfboard's right edge (x 14.5) is 0.25 mm from the rib (x 14.75) and 0.1 mm from the grommet seat (x 14.6)." | **Real but trivial.** A hand-cut perfboard can be +0.5 mm. **Smallest fix:** cut the board to x ≤ 14.0. The hole at 11.5 still keeps 2.5 mm of edge. |
| "With ~1.5–2 W inside (relay coils at 0.35 W each, the XIAO as a Thread router, buck loss), self-heating will bias the reading." | **Real in principle; already designed against.** The mitigations are the double-skin chamber with its own intake and exhaust, the sensor placed upstream of all heat sources, and a firmware per-relay offset (`SENSOR_SELF_HEAT_PER_RELAY_F10`). The residual has to be measured; the value is 0 until then (CLAUDE.md line 92). Nothing needs to change in the CAD. |
| "The chamber exhaust port opens into the electronics cavity, so warm air flows back." | **Overstated.** The port is at the chamber's top, and the flow is buoyancy-driven upward. The nearest heat source in the UI column (the XL7015) is 60 mm above the port. |
| "The F2→COM1 18 AWG wire at z 17 passes over the left wall screw (x −48..−32, y −13.5)." | **Overstated.** The backplate is screwed to the wall before the wiring is done, and the wire is flexible anyway. The right screw is clear of everything on the backplate, because the D-pad carrier lives in the cover. |
| "A USB-C plug won't seat: it is 7.5 mm from the receptacle mouth (x −61.5) to the outer wall face (x −69)." | **Mostly mitigated.** The 13 × 7.5 cut is oversized on purpose: a typical overmold (~12 × 6.5) can enter it, and USB is only for flashing. **UNVERIFIED** with the user's cable; try it on the fit print. |
| "The reference geometry shows the 12-way terminals spread over 66 mm (x −58.6..7.6). A real 12 × 5.08 block is about 61 mm, and the model's terminal boxes collide with the M3 heads at holes x −59 and 8." | **Modelling artefact, not a design fault.** The per-relay terminal boxes (line 801) are placeholders. On a real contiguous block, about 6 mm from each PCB end, the button heads (r ≈ 2.85, centred 3 mm in) clear it. **UNVERIFIED** until checked on the board, together with the terminal order. |

---

## 3. Enclosure, mechanical design, D-pad, mounting and printability

### Sound choices
- **The fit is empirically tuned, not guessed.**
  - CLR is 0.15 per side, after the 0.35 fit ring measured 0.30 real (line 23).
  - The snap bead protrudes 0.45 past the cover's inner wall (SNAP_R = 0.6, tip at 67.45 against an inner face at 67). The groove is 1.15 deep, leaving 0.85 of wall.
  - There are elephant-foot reliefs on the rim foot and on the cover's lead-in (lines 499–502, 573–576).
  - The fit ring confirmed that the cover snaps on and stays.
  - The single M3 × 10 bottom screw adds tamper and drop security. It goes into a boss (x 53..62.5) that sits clear of the chamber's intake vents (x ≤ 50), and its pilot is 8.85 mm deep.
- **Wall mounting tolerates the unknowns.**
  - The left slot is horizontal, ±8 mm, which covers 72–88 mm spacing. The user measured 80.
  - The right slot is vertical, ±3.5 mm, which allows about ±2.5° of levelling.
  - Both are #8 countersinks (head r 4.3) on 1.5 mm bosses, leaving ~2.1 mm of material under the cone.
  - The countersinks open upward, so they print with no overhang.
  - The window (34 × 11) is centred on the screw line, and its TPU grommet has a captive collar, a skirt that seals against the wall, and a slit membrane.
- **No inserts.** The M3 pilots are 2.6 mm and the M2 pilots 1.7 mm, thread-formed into PETG. Thread engagement is about 4.4 mm everywhere: M3 × 6 through a 1.6 mm PCB, and M2 × 6 into the 5.5 mm-deep SHT40 standoff.
- **Printability is respected throughout.**
  - The cover prints face-down. Every internal feature (divider, skin walls, posts, OLED frame) hangs from ZF and so grows straight up from the bed.
  - The vents are vertical slots in the side walls; only their 2 mm ends bridge.
  - The USB-C cut is a 13 mm bridge, which is easy in PETG.
  - The key openings carry an elephant-foot lip (line 513).
  - The backplate prints wall-face down. The skirt groove is a 1.8 mm ring bridge (line 117).
  - The arrow keys are one flexure plate on a single print plane (SFX_Z0). The centre key prints face-down with its 1.5 mm guide pins upward.
- **D-pad mechanics.**
  - The folded parallelogram arms (t 0.4, w 1.0) take δ/2 ≈ 0.75 each. For a fixed–guided beam of length ~10 mm, strain is about 3tδ/L² ≈ 0.9%. That is well below PETG's ~4% yield and fine for a thermostat's press count.
  - The rest position is set by the flange bearing on the cover's inner face, and the frame is clamped between the carrier tubes and the cover. That gives a positive datum.
  - Hard stops on the carrier (STOP_Z = 21.85) protect the switches from overtravel.
  - The centre key's guide pins were added after a real tilt-and-jam failure. They engage 2.5 mm at rest (pin bottom 19.35 against the column top 21.85), and their 0.15 per side socket clearance is sane.
  - Switches are retained by filament pins and crush ribs (0.1 bite). This was coupon-tested.
- **Clean thermal paths.** The XL7015 sits under the OLED on rails, with an air gap (≥3.5 mm) and zip-tie notches between its parts. The relay LEDs are Kapton-masked against vent glow.

### Likely criticisms
| Criticism | Assessment |
|---|---|
| "With the nubs gone, the flat pocket ceiling (z 24.2) meets the lever at its tip (s = −3, z 23.77) first. Of the 1.5 travel, 0.43 is gap, so the tip moves 1.07, which is ≈0.83 at s = 0. That is about equal to the ~0.85 click estimate in the comment (line 74)." | **Plausible: the model's margin is ≈0.** The mitigation is that the user coupon-tested the nub-less flexure and it clicks. "Switch lever travel to the click" is already on the measure-before-printing list (CLAUDE.md line 90). **Smallest fix if a key fails to click:** lower the pocket ceiling by 0.2 (FL_Z0 → ZF − 1.0, leaving a 0.23 tip gap, still no preload). Alternatively raise KEY_TRAVEL to 1.7, which bottoms 0.3 below the face. **Also:** the KEY_TRAVEL comment still describes the old nub arithmetic and should be updated. |
| "The XL7015 rail's zip-tie notch leaves only a 0.5 mm bridge (PLATE+1.0..+1.5)." | **Real but low-stakes.** A buck module needs little hold-down. **Smallest fix:** raise the rail to PLATE + 2.0, giving a 1.0 mm bridge. |
| "The chamber 'air gap' is 1.6 mm, not the 2 mm the comment says (divider face 17.3 against the skin at 18.9; −33.3 against −34.9)." | **Real but cosmetic.** 1.6 mm of still air insulates almost as well. Fix the comment. |
| "Slender posts: the SHT40 standoff is Ø5.2 × 11 and the rest posts Ø3.2 × 11." | **Overstated.** PETG at those aspect ratios is fine, and the load is a 0.5 g sensor board. |
| "Arrow-key flexure arms at 0.4 mm are two layers: fragile, and sensitive to layer height." | **Partly real.** They depend on a 0.2 mm layer height, which the coupon test validated. Note "0.2 layers" in the print notes. |
| "The case may not cover the old paint outline, and the SHT40 hole position is unmeasured." | **Open items, already listed** (BRIEF; CLAUDE.md lines 86–90). **UNVERIFIED.** The SHT40 standoff (centre 35.16, −42.7, r 2.6) sits within ~0.2 mm of the board edge, so confirm the hole position before printing. |

---

## 4. Documentation drift (real, trivial, fix before the next print)
- **HARDWARE_r1 line 101:** the BOM lists "M3×4 heat-set inserts". The design uses no inserts (code lines 33–35).
- **HARDWARE_r1 line 82:** it says "9-pin JST-PH". The plan is now inline pigtail pairs (6-pin D-pad, 4-pin OLED). The code's reference body is still "JST-PH 9p cover harness" (line 783) at x −36..−14, while the cover-harness tube ends at x 2.
- **HARDWARE_r1 lines 73–75:** the sensor is described as "about 18 × 12", "stands", and "seal the cable notch with putty". In the code it is measured at 12.56 × 10.5, lies flat, and goes through a TPU grommet.
- **CLAUDE.md line 85:** "about 75 mm apart, slots accept 65–85". The code says it was measured at 80 and the slots accept 72–88.
- **Code comment slips:**
  - line 39 says the module is "75 × 55", but it was measured at 73 × 50;
  - line 182 says "#6" in the unused `countersunk()` helper;
  - line 478 claims a "2 mm air gap".
- **dpad_module.py** uses a 6-pin 2.54 Dupont header, while the main case plans JST-PH. That is fine for a stand-alone module, but call it out so the two harness pinouts are not confused.

## UNVERIFIED (could not check from the files)
- the relay terminal order and the real terminal block position;
- whether 3.3 V triggers the opto at H;
- whether the XIAO C6 has a VBUS diode;
- the real XL7015 height and its IN/OUT ends: the 3.5 mm margin above it covers most cases;
- the fuse cap-off height: it only adds margin;
- the SHT40 hole position;
- whether the user's cable seats in the USB-C opening;
- the OLED active-area offset against the window (window centre y 38.75, glass centre 36.8);
- the lever travel to the click;
- whether the case covers the old paint outline.
