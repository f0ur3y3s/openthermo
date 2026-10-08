# Synthesis: openthermo v3, round 3 (wiring, placement, enclosure)

I recomputed every value below from `fusion_case_v3.py`. Frame: wall at z = 0, ZF = 25, ix = 67, iy = 55, rim faces at x ±66.85 (outer) and ±65.25 (inner), rim top z = 8, CTL_TOP = 7.6.

## Verdict

- **Printing: almost ready.** The architecture is sound and both reviewers agree on that.
  - Before printing the cover and backplate, make three small model edits: a fuse pocket, the bottom-screw pilot, and the XL7015 tie tunnels.
  - Also take three measurements first: the installed fuse-holder height, the XL7015 part heights, and the SHT40 hole position.
  - Nothing found needs a layout change to a printed part.
- **Wiring to the HVAC: not ready.** Two bench gates must pass first.
  - The relay terminal order (NO/COM/NC) must be confirmed.
  - The relays must pull in reliably from the 3.3 V GPIO.
  - One controller-board layout fix is also needed before soldering: move the 470 µF off the screw.
  - None of these is a printing issue.

## Prioritised action list

| # | Action | When | Severity |
|---|---|---|---|
| 1 | Confirm the relay NO/COM/NC order on the real terminals and paint-mark the four NO screws | Before landing any field wire | Blocker (wiring gate) |
| 2 | Prove the relays pull in when IN is driven from the XIAO GPIO at 3.3 V, and measure the IN current | Bench step 2 | Major (gate) |
| 3 | Measure the installed fuse-holder height. Add a 1.0 mm pocket to the cover's inner face over the fuses | Before printing the cover | Major |
| 4 | Move the 470 µF axis from x = 7 to x = 4 so a driver can reach the (11.5, −49) screw | Before soldering the controller | Major |
| 5 | Remove the relay-cable connector: solder it at the controller and screw it at the module. Give the OLED and SHT40 4-pin cables the same pin order | Before making the harness | Major |
| 6 | Bottom M3 × 10: move the pilot end to y −45 and the boss end to y −43 | Before printing the backplate | Minor |
| 7 | XL7015 tie tunnels: cut them down into the plate (z 1.5..3.5, 1.0 roof), or drop them and use foam tape | Before printing the backplate | Minor |
| 8 | Measure the XL7015 tallest-part height within x 26..56; it must be ≤ 13.4 mm above its PCB | Before printing the cover | Minor (check) |
| 9 | D-pad: confirm the coupon used today's FL_Z0 and STOP_Z and that every key clicks before it bottoms. If not, set FL_Z0 = ZF − 1.0 | Before printing the key plate | Minor (check) |
| 10 | Fuse-holder header pins must be **soldered** to the clip legs, because CA glue does not conduct. Ohm-check them | Bench | Minor (own finding) |
| 11 | Everything else: strain relief, I2C pull-ups, D6 resistor, SHT40 standoff, perfboard edge, docs | See below | Minor |

---

## Rulings

### Electrical

**B1. The relay NO/COM/NC order is unverified (critic B1; the sympathizer agrees). CONFIRMED, Blocker for wiring (not for printing).**
- `build_wiring` lands the field wires at `kx − 5`, which assumes the order NO, COM, NC from the left. The brief says this is unconfirmed.
- If the order is wrong, Y1, G, O and W are on NC. They are then energised whenever the relay is off: at boot, during a fault and through the lockout. That defeats every hard rule.
- Bring-up step 2 as written catches this only if the person doing it infers NO from the behaviour.
- **Fix:** add a hard gate to step 2:
  1. With the module unpowered, each terminal you plan to use must read **open** to COM.
  2. The third terminal must read **closed** to COM; that one is NC.
  3. Drive each IN high and confirm the used terminal closes.
  4. Paint-mark the four NO screws.
  5. Update `kx` and the landing x positions once the block has been measured.

**M3 / sympathizer. The H-trigger module may not pull in from 3.3 V. CONFIRMED (UNVERIFIED for this board), Major as a bench gate.**
- A failure here is fail-safe (the relay stays off), but it breaks function.
- On H/L modules the IN path is often a resistor, the opto LED and an indicator LED in series. That is about 3.0 V of LED drop, so 3.3 V may give well under 1 mA.
- **Fix:** run step 2 from a XIAO GPIO, not from 5 V.
  - Pass: at least 1.5–2 mA of IN current and solid pull-in with every relay energised.
  - If it is marginal, set the jumpers to **L** and add one NPN per channel (2N3904, 2N2222 or 2N7000). Connect the collector to IN and the emitter to GND, feed the base from the GPIO through 2.2–4.7 k, and keep the 10 k pull-down on the GPIO.
  - With this circuit, GPIO high still means energised, and a floating or low GPIO still means off.
  - **Do not** use an inverting pull-up stage in H mode. That would turn every relay on at boot.
  - Update the CLAUDE.md "H-trigger" wording if you change the jumpers.

**M1. Two 6-pin JST-PH cables can be cross-mated. CONFIRMED, Major. The critic overstated the consequences.**
- I traced both swaps.
- **Module-side relay plug on the controller's D-pad pigtail:** this is harmless. The module's DC+ is fed only through that cable, so the module is unpowered and no relay can pull in. The critic's "uncommanded relay" and "5 V reaches a GPIO" do not happen in this half.
- **D-pad plug on the controller's live relay pigtail:** this is the real hazard. A key press ties the controller's 5 V, GND or an IN-driving GPIO together. Depending on the pin order, that is 5 V into an ESP32 GPIO, which kills the XIAO, or a short of the 5 V rail.
- **The two 4-pin cables:** the module pin orders differ (SSD1306 is GND-VCC-SCL-SDA, SHT40 is VIN-GND-SCL-SDA). A swap reverses power to a module unless the cable pin order is made the same.
- **Fix:**
  - Don't put a connector on the relay cable. Solder it at the controller and screw it at the 6-way block, which is already removable. That leaves one 6-pin PH pair in the box.
  - Wire both 4-pin pigtails in the same order, for example GND, 3V3, SDA, SCL. A swap is then harmless, because both modules are on the same bus.
  - Paint pin 1 on every housing.
  - The Dupont singles in `dpad_module.py` are harmless when mis-ordered, because they are all inputs plus GND. They become a hazard only if they could reach the relay cable.

**M2. A failed XL7015 puts the bus on the 5 V rail. CONFIRMED in mechanism, but Minor.**
- If the buck's high side shorts, 33–39 V reaches the module. Its driver transistors (Vceo about 25 V) can avalanche and pull all four relays in.
- But four 5 V coils at 39 V draw about 0.55 A each, about 2.2 A in total. F1 (T1A) then opens within seconds to a couple of minutes, and everything drops. That is fail-safe.
- The air handler's and outdoor unit's own limit and pressure controls stay in the loop throughout.
- It is a rare single-point fault, and it does not lead to fire.
- **Fix:**
  - Lock the trimpot with nail polish after setting 5.00 V.
  - Optional: a 5V6 1 W zener across the 5 V rail with a 500 mA polyfuse ahead of it (parts are probably not on hand).
  - The delay-on-break timer on Y, already listed as "not covered", is the real hardware backstop.

**M8. F2 (T1.6A) is sized close to the Heat + aux load. OVERSTATED.**
- A typical Y1 + G + W load is about 0.6–1.2 A.
- The 75 % derating rule applies to UL 248-14 fuses. A 5×20 IEC 60127 T fuse carries its rated current indefinitely and must not open within 1 h at 1.5 × In.
- The transformer (40 VA, about 1.67 A) limits the steady load near F2's rating anyway.
- The "1.25 A contactor pickup" in HARDWARE is inrush, which a T fuse rides through.
- **Fix (bench):** clamp-meter R in each call (heat, cool, heat + aux, e-heat).
  - Keep T1.6A if the steady current is ≤ 1.1 A.
  - Otherwise fit T2A. 18 AWG and the 10 A contacts allow it.

**m3. D6 is GPIO16, which is U0TXD. CONFIRMED, Minor.** Holding Right during reset shorts the ROM's TX driver to GND.
- **Fix:** add a 1 k series resistor in the D6 switch line, and turn off the ROM and bootloader UART log (console on USB-Serial-JTAG).

**m2. The I2C pull-ups may leave with the cover. CONFIRMED (UNVERIFIED), Minor.**
- **Fix:**
  - Check both modules for pull-ups.
  - If the SHT40 module has none, fit 10 k (not 4.7 k) to 3V3 on the controller. With the OLED's 4.7 k in parallel that gives about 3.2 k, which keeps the sink current under 3 mA.
  - Make sure `i2c_bus` does 9-clock bus recovery.

**m4. Relay pin state at reset. OVERSTATED.**
- GPIO18 and GPIO21 may have weak pull-ups at reset (they are SDIO pins). Against the 10 k pull-down, the pin sits at about 0.6 V.
- The opto plus LED string needs about 2.5–3 V, so nothing conducts.
- **Fix:** optional; scope the IN pins through a reset and a flash during bench step 4.

**m5. EMI and ESD near the UI and the contacts. OVERSTATED (optional).** The coil loads are well under 1.5 A on 10 A contacts, and firmware debounces the keys.
- **Fix, only if resets appear:** add 100–330 Ω series resistors on the D-pad lines, or an RC snubber (47 Ω + 100 nF) across Y1 and O.

**Sympathizer's "opto isolation is defeated because DC− = C". REJECTED as a criticism.** Everything shares the Class 2 reference by design, and half-wave rectification with C = GND is correct.

**The sympathizer's other electrical points are also confirmed sound:**
- Two 18 AWG wires in one terminal are fine. A looped single wire, stripped mid-span, is neater.
- The XIAO VBUS diode is harmless either way.
- TVS standoff 43.6 V > the 41.7 V peak at 30 VAC unloaded.
- Ripple is about 2.5 V p-p at about 70 mA of bus draw, which is irrelevant.

**Own finding: the fuse-holder leg mod.** The brief says the header pins are "soldered/CA-glued". Cyanoacrylate does not conduct. F2 carries up to about 1 A, and a pressure-only contact will heat up and become intermittent.
- **Fix:** solder each pin to its leg, do a tug test, and check that the resistance from clip to pin is ≤ 0.05 Ω.

### Placement and routing

**M4 / sympathizer. The 470 µF covers the (11.5, −49) screw. CONFIRMED, Major (build).**
- The cap body spans x 2..12 and y −50.5..−29.5. At x = 11.5 its underside is at z 10.42, above the M3 button head (top 9.25). So the head fits underneath, but **no driver can reach it** once the cap is soldered.
- **Fix:** put the cap axis at **x = 4**, giving a body at x −1..9.
  - The driver shaft at x 10.5–12.5 then clears it by 1.5 mm.
  - The cap stays 0.5 mm clear of the R-C terminal in y (it ends at −29.5; the terminal starts at −29).
  - It is well clear of the fuses (x ≤ −9.2).
  - This also frees x 10..14 for a pad strip.
  - Fallback: run three screws and omit that one.
  - Reference-only change: no printed part changes.

**The rest of M4 (harness landings on screw heads, the cover harness overlapping the R-C terminal by 0.36 mm, the stale 9-pin block). CONFIRMED as model artefacts, Minor.**
- **Fix:** define a pad strip at x 10..14:
  - SHT40, 4 pads, at y −42..−36;
  - XL7015, 3 pads, at y −34..−29.
  - Keep pads ≥ 3 mm from screw centres.
  - Delete the "JST-PH 9p" reference body.

**Sympathizer: the perfboard's right edge is 0.25 mm from the rib (14.75) and 0.1 mm from the grommet (14.6). CONFIRMED, Minor.**
- **Fix:** cut the board to x ≤ 14.0. That leaves 2.5 mm from the hole at 11.5 to the edge.

**M7. The relay harness in the left channel is unsupported and sits on the pinch line. Partly CONFIRMED, Minor (the shear claim is OVERSTATED).**
- The bundle (r 1.83, x −65.03..−61.37, z 9.6..13.3) lies entirely **inboard** of the rim inner face (−65.25) and above the rim top. It is not in the cover wall's path.
- But it is unsupported. If slack loops outward over the rim before the cover goes on, it gets trapped against the rim's outer face, where the gap is only 0.15.
- **Fix (bench):**
  - Tack the bundle to the side of the 6-way block and the PCB edge with Kapton or a dab of hot glue.
  - Look along the rim before closing.
- **Optional model fix:** raise the left rim to z 13 over y −18..−6 and y 22..40. This keeps clear of the snap bead (y ±19) and the USB cut.

**M5. The inline connector pairs and the cover slack have no home. CONFIRMED, Minor. The critic overstated the lack of space; the sympathizer is right that room exists.**
- A 6-pin PH pair (about 14 × 4.5 × 12) does not fit the x ≈ 14 channel, which is 6.25 wide.
- But under the carrier there is a free volume at x 17..60, y −30..20, z 4.5..10.5. It is bounded by the right wall-screw boss top (4.5) and the switch tails (about 10.5).
- A pair lying flat (4.5 tall) fits there with about 3 mm to spare. It is exposed whenever the cover, with the carrier attached, comes off.
- **Fix:**
  - Model the D-pad (6) and OLED (4) pairs lying flat at about x 22..40, y 2..18. This is reference only.
  - Give the cover-side pigtails about 80 mm of slack, stowed as a loop in that same volume.
  - Add a stick-on tie point or a dab of hot glue on the backplate.

**m1. The cover harness has no strain relief. CONFIRMED, Minor.**
- **Fix:**
  - Hot-glue the wires to the OLED PCB next to the pads.
  - Zip-tie or glue the D-pad bundle to the carrier, so that flexing happens in the wire.

**m10. The field-cable strain relief is off the cable path. OVERSTATED.**
- The tie bridge (x −22.5..−19.8, with a tunnel at z 4.5..6.3) cinches a cable that rises through the **left end** of the window against the bridge's side face.
- The conductors fan out to x −6..+6.5 after the jacket ends.
- **Fix:** note in the assembly steps "feed the cable through the left end of the window". Cap B and the spares.

**m11. The reference terminals clash with the module holes. OVERSTATED, a modelling artefact.**
- On a real contiguous 12 × 5.0 block (about 61 mm) there is about 6 mm at each end of the 73 mm board. The M3 button heads (r 2.85, centred 3 mm in) clear it, just.
- **Fix:** measure the block position and relay pitch together with item B1, then update `kx`.

**m15. There is only a 3 mm gap under the perfboard. CONFIRMED, Minor.**
- XIAO female header tails (about 3 mm), terminal pins and header-pin stubs can touch the backplate.
- **Fix:**
  - Trim every lead to ≤ 2 mm.
  - Run the 18 AWG links (R, C and F2) on the component side.
- Do **not** lower the controller bosses. That would trade against this gap.

**The relay top clearance (1.9 mm), the wall screws reachable with the boards fitted, the USB-C alignment and the bus margins are all fine.** I checked each one. The USB-C fit with the user's own cable is UNVERIFIED; try it on the fit print.

### Enclosure, mechanics and thermal

**M6. The fuse holders have 0.4 mm clearance to the cover. CONFIRMED, Major (it gates the cover print).**
- With the cap on, the holder top is 7.6 + 17 = 24.6, against ZF 25.
- The header-pin mod can add up to about 2.5 mm if a pin spacer or shoulder sits under the holder, which gives about 27.1. The cover would then not close.
- The sympathizer's "cap-off only adds margin" is true, but it ignores the leg mod.
- **Fix:**
  - Measure the installed height from the perfboard top to the highest point, H.
  - In the model, cut a **1.0 mm pocket into the cover's inner face** at x −37..−8, y −52.5..−29.5, z 24..25.
    - That leaves a 1.0 mm skin, about five layers.
    - It prints with no supports, because the cover prints face-down and the pocket opens upward.
    - The front face is untouched.
  - This allows H ≤ 17.9 with 0.5 mm of margin.
  - If H is greater than that, strip the header spacers or run the holders cap-off.
  - Do not lower the bosses (see m15).

**m7. The bottom M3 × 10 has zero thread margin. CONFIRMED, Minor.**
- The head sits at y −57 and the tip at −47, which is exactly the pilot floor (L564). The plastic engagement is 7.85 mm, so there is plenty of thread but the screw can bottom out and strip.
- **Fix:** pilot end −47 → **−45**, boss end −45 → **−43** (L546 and L564).
  - The boss stays clear of the chamber skin (y −36.1).
  - Alternatively, use an M3 × 8 with no model change.

**m8 / sympathizer. The XL7015 tie tunnels are 1.0 mm tall with a 0.5 mm roof. CONFIRMED, Minor.**
- Typical 2.5 mm ties are 1.0–1.2 mm thick, so they will not thread through.
- The sympathizer's fix (raise the rail) eats the XL7015 height budget (see m9).
- **Fix:** change the notch z range in L556 to `(PLATE − 1.5, PLATE + 0.5)`.
  - That makes the tunnel 2.0 tall with a 1.0 roof, and leaves 1.5 mm of plate under it.
  - The rail height is unchanged.
  - The roof is a 5 mm bridge, which is fine.
  - Or delete the notches and stick the board down with foam tape.

**m9. The XL7015 height budget. OVERSTATED (measure).**
- The two reviewers' numbers agree: the OLED frame bottom (z 20) leaves **13.9 mm above the XL PCB top** (15.5 from the board's underside).
- But the frame only covers x 26..56. The board spans x 16.5..60.5, so parts at either end (often the IN and OUT caps) have about 18 mm of room up to ZF.
- **Fix:** measure on arrival. If a part inside x 26..56 is more than 13.4 mm tall:
  - slide or turn the board so the tall cap falls outside the frame footprint; or
  - lay the cap over.
- Do not raise the rails.

**The D-pad travel margin without nubs (sympathizer). CONFIRMED, Minor (check).** I recomputed it:
- Lever tip at z 23.77, pocket ceiling at FL_Z0 24.2, so the gap is 0.43.
- Over KEY_TRAVEL 1.5 the tip moves 1.07, which is **0.83 at s = 0**, against the stated click of about 0.85. In the model the margin is about zero.
- The coupon result is the mitigating evidence, but it only counts if the coupon had today's FL_Z0 and STOP_Z.
- **Fix:** print the key plate, the carrier and a cover-face coupon first.
  - If any key, including the centre key, fails to click before it bottoms, set `FL_Z0 = ZF − 1.0`. That gives a 0.23 tip gap, 0.98 of travel at s = 0, and still no preload.
  - Update the stale KEY_TRAVEL comment (L71–75), which still describes nubs.
  - Note "0.2 mm layers" for the 0.4 mm arms.

**m6. The SHT40 is thermally tied to the backplate. CONFIRMED (estimate), Minor.** My numbers match the critic's.
- Conduction through the supports is about 8.8e-4 W/K (standoff r 2.6 with the steel screw, plus two posts at r 1.6). Convection from the board is about 1.3–2.6e-3 W/K.
- So the sensor reads **25–40 %** of the way from air temperature to backplate temperature.
- The backplate tracks the wall surface. On an exterior wall that surface is about 1 K below room air in winter if the wall is insulated, and up to about 5 K if not. So the error is about 0.3–2 K, and it drifts with the weather.
- **Fix in the model:**
  - Standoff r 2.6 → 2.0.
  - Rest posts r 1.6 → 1.2. Keep both, for three-point support.
  - This brings the error fraction down to 16–28 %.
- **On the bench:** log against a reference thermometer over a cold night before trusting the offset.
- Optional: a nylon M2 screw.

**m12. The SHT40 cable grommet bore is too tight. CONFIRMED, Minor.**
- Four 26 AWG wires at OD 1.3–1.4 need a circle 3.14–3.38 mm across; the bore is 3.2.
- The code has no slit, although the L122 comment says there is one.
- **Fix:**
  - Set SG_CH_D to 3.6.
  - Knife-cut a vertical slit from the block top to the bore (the cover's squeeze closes it).
  - Make the L122 and L130 comments agree.

**m13. Countersunk #8 screws in PETG slots: creep and wedging. OVERSTATED (optional).** The cover's snap fit, not the screw clamp, carries the load.
- **Fix:** if pan-head screws, or the old Braeburn screws, are to hand, they sit on the cone rim fine. A TPU washer is optional.

**Sympathizer: the SHT40 standoff is 0.2 mm past the board edge, and the hole position is unmeasured.** CONFIRMED as a measure-before-printing item for the backplate.

**Sympathizer: the chamber gap comment says 2 mm, but the real gap is 1.6 mm.** CONFIRMED, documentation only.

**Sympathizer: the OLED window centre (y 38.75) is offset from the glass centre (36.8).** UNVERIFIED and probably intentional, since the active area usually sits toward the end of the glass away from the flex cable. Check it on the fit print.

**Sympathizer: the posts are slender, and the chamber exhaust feeds the UI column. Both OVERSTATED.** The two reviewers agree.

---

## Fix in the model before printing

1. **Cover:** add a 1.0 mm-deep pocket on the inner face over the fuses (x −37..−8, y −52.5..−29.5, z 24..25). Set FUSE_H to the measured installed height.
2. **Backplate, bottom screw:** pilot to y −45 (L564) and boss to y −43 (L546).
3. **Backplate, XL rails:** tie notch z → `(PLATE − 1.5, PLATE + 0.5)` (L556).
4. **Backplate, SHT40 supports:** standoff r 2.0, rest posts r 1.2 (L543 and L545). Confirm the hole position first.
5. **TPU sensor grommet:** SG_CH_D = 3.6.
6. **Conditional, D-pad:** `FL_Z0 = ZF − 1.0` if the check in action 9 fails.
7. **Optional, backplate:** local left-rim fences to z 13 at y −18..−6 and y 22..40.
8. **Reference and wiring only (not printed):**
   - 470 µF axis at x = 4;
   - perfboard right edge at x 14.0;
   - delete the 9-pin JST block;
   - add the 6-pin and 4-pin PH pairs lying flat under the carrier at about x 22..40, y 2..18;
   - a right-end pad strip at x 10..14;
   - `kx` and the terminal order once measured.

## Check on the bench / measure

**Before printing:**
- the installed fuse-holder height H, which must be ≤ 17.9 with the pocket;
- the tallest XL7015 part within x 26..56, which must be ≤ 13.4 above its PCB;
- the SHT40 hole position;
- the D-pad click before it bottoms, on a coupon with the current parameters;
- the wall-screw spacing and the paint outline (already open).

**On the fit print:**
- the USB-C plug seats;
- the OLED active area is centred in the window.

**Before wiring:**
1. The relay NO/COM/NC gate (B1). Paint-mark NO.
2. Drive IN from the 3.3 V GPIO. IN current must be ≥ 1.5–2 mA with solid pull-in, or switch to L-mode plus NPN.
3. The fuse header pins are soldered, not glued, and ≤ 0.05 Ω.
4. All leads under the perfboard trimmed to ≤ 2 mm, with the 18 AWG runs on top.
5. Check both modules for I2C pull-ups and add 10 k if none. Fit the 1 k resistor on D6.
6. Relay cable hard-wired. 4-pin pigtails in the same order. Pin 1 painted.
7. XL7015 at 5.00 V, then lock the trimpot.
8. Optional: scope the IN pins through a reset and a flash.

**After installation:**
- Clamp-meter R in each call; F2 stays at T1.6A if the steady current is ≤ 1.1 A, otherwise T2A.
- Log the SHT40 against a reference thermometer overnight, then set the self-heat and wall offsets.

## Docs to update

- **HARDWARE_r1, bring-up step 2:** replace it with the NO/COM/NC gate wording, and drive IN from 3.3 V (not "from 5 V").
- **HARDWARE_r1, UI:** "9-pin JST-PH" → a 6-pin D-pad PH pair plus a 4-pin OLED PH pair. Add the pin orders and the "relay cable hard-wired" rule.
- **HARDWARE_r1, Sensor:**
  - 12.56 × 10.5, lies flat, M2 × 6 plus two posts, TPU grommet. Drop "putty".
  - Note the backplate-coupling calibration step.
- **HARDWARE_r1, BOM:**
  - remove "M3×4 heat-set inserts" (threaded straight into PETG);
  - 9-pin → PH 6 + 4;
  - add the 1 k resistor for D6;
  - add the optional 10 k pull-ups and the optional NPN drivers.
- **HARDWARE_r1, power:** note that the 1.25 A "contactor pickup" is inrush, and the F2 measurement rule.
- **CLAUDE.md status:**
  - "about 75 mm apart, 65–85" → measured 80, slots 72–88;
  - add the fuse height and the SHT40 hole to "measure before printing".
- **Code comments:**
  - L39: "75 x 55" → 73 × 50.
  - L71–75: KEY_TRAVEL and the nub arithmetic are stale.
  - L122: the slit; make it agree with L130.
  - L478: "2 mm air gap" → 1.6.
  - L181: delete the unused #6 `countersunk()`.
- **dpad_module.py:** note that its Dupont pinout is separate from the main-case PH pinout.
