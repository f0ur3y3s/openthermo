# Critic review: openthermo v3 wiring, placement, mechanical (round 3)

Geometry is computed from `/home/claude/thermo/fusion_case_v3.py` (cited as `L<n>`). Frame: wall at z=0, ZF = 25 (cover inner front face).
Derived values: cover inner half-extents ix=67, iy=55. Backplate rim outer x = ±66.85, rim inner x = ±65.25, rim top z = 8. Perfboard top CTL_TOP = 7.6. Switch body top SW_Z1 = 20.02. Carrier underside CAR_Z0 = 12.82.

---

## BLOCKER

### B1. The relay NO/COM/NC order is unverified, and a wrong guess breaks every hard safety rule
- **Where:** `build_wiring` L364-383 puts each field wire at `kx - 5` (left position = NO) and the COM jumpers at the centre of each 3-way block. The brief says "order on the board not confirmed".
- **Why it matters:** if the board order is NC-COM-NO, Y1, G, O and W land on NC. They are then energised while the relay is off, which covers boot, faults, brown-out and the 5-minute lockout. The compressor runs with no minimum-off, O is on in heat, and a fault does not drop anything. All the firmware interlock work is defeated by one terminal. Bring-up step 2 ("check continuity from COM to NO") only catches this if NO was identified correctly in the first place.
- **Fix:** make this a hard gate before wiring:
  - With the module unpowered, check that each field terminal is OPEN to COM and that the third position is CLOSED to COM (that one is NC).
  - Power DC+ only, leave every IN low, and repeat the check.
  - Label the four NO screws on the block with a paint pen.
  - Add the check to HARDWARE bring-up step 2 in exactly these words.

---

## MAJOR

### M1. Two 6-pin JST-PH cables (relay and D-pad) can be cross-mated, and one pairing puts 5 V on an ESP32 GPIO
- **Where:** brief, "Connectors": a 6-pin relay cable (5V GND IN1-4) and a 6-pin D-pad cable (5 switches + GND). Both come back to the controller board near (-50,-22) and (2,-24).
- **Why it matters:**
  - **Relay-side plug on the D-pad-side controller pigtail:** 5 V reaches one of D0/D8/D9/D6/D7. The C6 is not 5 V tolerant, so the XIAO is damaged.
  - **D-pad-side plug on the relay-side controller pigtail:** a key press can short 5 V to GND, or tie an IN line to 5 V, which energises a relay with no firmware involvement.
  - **The two 4-pin cables (OLED and SHT40):** if their pin orders differ, a swap reverses power to a module. Common SSD1306 boards are GND-VCC-SCL-SDA; SHT40 boards are VIN-GND-SCL-SDA.
  - **dpad_module.py:** it uses six single-pin Dupont housings, which can be plugged in any order.
- **Fix:**
  - Don't put a connector on the relay cable at all. The relay end is already screw terminals, so solder the cable at the controller and screw it at the module. That leaves one 6-pin cable in the box.
  - Give the OLED and SHT40 cables identical pin order (GND, 3V3, SDA, SCL). A swap is then harmless, because both are on the same bus.
  - Mark pin 1 on every housing.

### M2. A failed or misadjusted XL7015 puts the 33-39 V bus on the 5 V rail, and the relays can then pull in without being commanded
- **Where:** power chain in the brief and HARDWARE. Nothing clamps the 5 V rail. The trimpot sits exposed on the backplate (L777-780).
- **Why it matters:**
  - A buck high-side short, or a trimpot turned to maximum, puts about 39 V on the relay module and the XIAO 5V pin.
  - The module's input transistors (typically S8050, Vceo 25 V) and the XIAO LDO fail, and GPIO state becomes undefined.
  - Coils can energise in any combination, including Y1+O+W, with no firmware in the loop.
  - This is a single-point failure that bypasses both `hvac_logic` and `relays_guard`.
- **Fix:**
  - Put a 5 V crowbar on the 5 V rail, for example an SMBJ5.0A TVS or a 5V6 1 W zener. Feed the rail through a 500 mA polyfuse or a small fuse so the clamp opens it, rather than relying on F1.
  - Lock the trimpot with nail polish once it is set.
  - Fit the delay-on-break timer on Y that CLAUDE.md already lists as "not covered".

### M3. A 5 V H/L relay module may not trigger reliably from 3.3 V logic (UNVERIFIED for this board)
- **Where:** brief: H-trigger, IN driven from XIAO GPIO at 3.3 V. HARDWARE bring-up step 2 says to toggle "each IN by hand", which in practice usually means from 5 V.
- **Why it matters:** on many H/L-jumper modules the IN pin drives a resistor plus the opto LED plus an indicator LED in series. That totals about 1.2 V + 1.8-2.0 V before the resistor. At 3.3 V the opto current can be under 1 mA, which gives marginal or temperature-dependent pull-in and chatter.
- **Fix:**
  - Test each IN from the 3.3 V pin, not 5 V, and measure the IN current. Aim for at least 2 mA.
  - If it is marginal, short the series indicator LED, or use a 2N7000 / NPN low-side stage driven at 5 V. Keep the 10 k pull-downs on the GPIO side.

### M4. The right end of the controller board is over-committed: a screw sits under the 470 µF, and three harnesses land on screw heads or the cap
- **Where:**
  - Controller hole (11.5,-49) (`CTL_HOLES` L50). The 470 µF lies along y at x=7, r=5, so it spans x 2..12 and y -50.5..-29.5 (L781). The cap body sits directly over that M3 head (head x 8.65..14.35). The cap's underside at x=11.5 is z≈10.4, against a head top of 9.25.
  - The XL7015 3-wire starts at (12,-24,8.9) (L402), on top of the other screw head at (11.5,-23).
  - The SHT40 4-wire ends at (12,-40,9.2) (L412), against the cap.
  - The cover harness (r 2.36) ends at (2,-24,~10) (L420) and overlaps the R-C terminal body (x -10..0, L782) by 0.36 mm.
- **Why it matters:**
  - The board cannot be unscrewed or reinstalled without desoldering the cap.
  - Four cable landings (3 + 4 + 10 wires, plus the R and C field wires) share an area about 12 × 30 mm beside the 24 VAC terminal.
  - The modelled 9-pin JST (L783, x -36..-14) is not where the cover harness actually lands.
- **Fix:**
  - Move that screw hole to x ≈ 0 (re-check the R-C terminal), or move the cap to stand vertically elsewhere. With the cap lying flat its top is 17.6, and standing it is 21.6+ to 27.6 against ZF 25, so lying flat is fine but it must be somewhere else.
  - Define one landing strip of pads per harness and draw the connectors at their real size.

### M5. The inline connector pairs and the cover-removal slack have no modelled home
- **Where:** brief: inline PH pigtail pairs for the relay (6), OLED (4), D-pad (6) and optional SHT40 (4). The model has only bundle tubes (L384-420) plus one stale 9-pin JST block (L783).
- **Why it matters:**
  - A mated 6-pin PH inline pair is about 14 × 4.5 × 12 mm, and a 4-pin pair about 10 × 4.5 × 12.
  - The x≈14 channel is 6.25 mm wide: from the module edge at x=11 to the carrier at x=17.25, and only z 3..12.8 under the carrier. A 6-pin pair does not fit there in either orientation.
  - Taking the cover off far enough to reach the connectors (cover depth 27 mm, plus setting it down) needs roughly 80-120 mm of slack in the 10-wire harness. All of that has to be stowed somewhere when the cover is closed.
  - The likely result is loose loops pushed into the relay area or under the carrier, where they get pinched (see m1).
- **Fix:**
  - Place both cover-harness connector pairs explicitly. One candidate is the free volume above the wire window at x 8..17, y -19..-8, z 5..24.
  - Model the slack loop, and add a tie point on the backplate.
  - Or fit a single PH header on the perfboard, so only one housing floats.

### M6. The fuse holders have 0.4 mm clearance to the cover, and the header-pin leg mod probably raises them
- **Where:** `FUSE_H` = 17 (L43). Holder top = CTL_TOP + 17 = 24.6, against ZF = 25 (L785-786).
- **Why it matters:**
  - 0.4 mm is inside the stack-up of perfboard thickness (1.2-1.6), boss print tolerance, and solder under the holder.
  - The user soldered or CA-glued male header pins to the legs. If the header's 2.5 mm plastic spacer is still on, or the holder sits on the pin shoulders, its top is about 27, which is past the cover face. The cover would then either not close or press on the fuse caps through the front face.
- **Fix:**
  - Measure the installed height on the board, not the loose part.
  - If it is over 24, lower the controller bosses from 6.0 (L541: `CTL_TOP - 1.6`). There is room, because the module bosses are also 3 mm and nothing else needs the clearance.
  - Or fit low-profile holders with no cap, which are open clips about 10 mm tall. These are fine inside a closed case on a 24 VAC circuit.

### M7. The relay harness in the left channel has no support and sits on the cover-rim pinch line
- **Where:**
  - Bundle diameter 3.66 at x = -63.2, z 9.6..13.3 (L385-389).
  - The gap from the module PCB edge (-62) to the rim inner face (-65.25) is 3.25 at z < 8. That is smaller than the bundle, so the model floats it above the rim, where nothing holds it.
- **Why it matters:**
  - The bundle will drop onto the rim top (z 8, x -66.85..-65.25), or past the rim outside face.
  - The cover's inner wall slides down outside the rim with a 0.15 mm gap (CLR). A wire lying over the outer edge of the rim gets sheared by the cover, possibly on every reinstall.
  - The same applies to the top run at y = 48.5 if it wanders toward the rim at y = 53.25.
- **Fix:**
  - Add two or three tie-down loops or clips on the inside of the left rim, for example at y = 0 and y = 30, z 3-8.
  - Or raise the rim on the left wall to z 14 locally, so the bundle is held between rim and module. The cover still slides outside it.
  - Or route the bundle at x ≈ -60.5, over the module PCB margin and outboard of K1 (x -58.75), which leaves 2.4 mm.

### M8. F2 is sized close to the steady load in Heat + aux (UNVERIFIED loads)
- **Where:** F2 T1.6A feeds all four COMs, and W drives W1 and W2 jumpered at the wall.
- **Why it matters:**
  - A typical heat-pump air handler draws roughly: contactor 0.25-0.35 A held (1.5-2.5 A inrush), blower relay or ECM 0.1-0.2 A, aux sequencers 0.25-0.4 A each × 2.
  - Heat + aux (Y1+G+W) therefore runs about 1.0-1.4 A steady inside a warm box. That is 65-90 % of a T1.6A, past the usual 75 % derating, with contactor inrush on top.
  - A nuisance blow fails safe electrically, but it means no heat in winter.
- **Fix:**
  - Clamp-meter the R current in each call (heat, cool, heat + aux, emergency heat) before closing up.
  - Size F2 at 1.5 × the worst steady current. T2A or T2.5A is likely; 18 AWG and the 10 A contacts allow it, and the air handler's own 3-5 A fuse stays upstream.

---

## MINOR

### m1. The cover harness drapes loosely and can be pinched as the cover goes on
- **Where:** the D-pad bundle runs at z 9.5 under the carrier (L418). That is 1.8 mm above the right wall-screw boss (top z 4.5) and only about 1.5 mm below the carrier floor at z 12.8.
- **Why it matters:** the solder joints on the OLED pads (header removed) and on the switch tails have no strain relief. Repeated removal flexes them, and OLED pads lift easily.
- **Fix:** add a tie or clip on the carrier and on the OLED frame (L466-474), so the flex happens in the wire and not at the joint.

### m2. The I2C pull-ups may leave with the cover
- **Where:** the SHT40 cable and the cover OLED share one bus.
- **Why it matters:**
  - If the SHT40 module has no pull-ups (UNVERIFIED), unplugging or loosening the OLED connector removes the bus pull-ups. The SHT40 reads then fail, and after 2 minutes there is a sensor fault and all HVAC stops.
  - Hot-unplugging mid-transaction can also leave SDA held low.
- **Fix:**
  - Fit 4.7 k pull-ups to 3V3 on the controller board, and check the parallel value with the module resistors fitted (at least about 2 k).
  - Make sure `i2c_bus` does 9-clock bus recovery.

### m3. D6 (GPIO16 / U0TXD) with a switch to GND
- **Why it matters:**
  - Holding Right during reset shorts the ROM and bootloader TX driver to GND, roughly 20-40 mA.
  - If the IDF console or bootloader log is left on UART0, logging contends with the button.
- **Fix:**
  - Fit a 1 k series resistor on D6.
  - Set `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` or none, and turn off the ROM and bootloader UART log.

### m4. Pin state at reset (UNVERIFIED)
- **Where:** relay pins GPIO1, 2, 21 and 18. Some C6 pads (SDIO-capable GPIO18-23) may have weak pull-ups at reset.
- **Why it matters:** with a 10 k pull-down and a roughly 45 k internal pull-up, the pin sits at about 0.6 V. That is below opto threshold, so it is safe, but nobody has measured it.
- **Fix:** scope each IN through a reset and a flash, with the module connected. Add this to the bench log check.

### m5. ESD and EMI near the UI and contacts
- **Where:**
  - The 10-wire cover harness runs in the same channel as the 33-39 V bus wire of the XL7015 cable (L402-407).
  - Relay contacts switch inductive 24 VAC loads (contactor, reversing-valve solenoid) with no snubber.
  - The D-pad lines go straight to GPIOs.
- **Fix:**
  - Put 100-330 Ω in series on the D-pad lines at the XIAO.
  - Keep I2C at 100 kHz.
  - If resets show up during contactor drop-out, add RC snubbers or MOVs across the NO-COM of Y1 and O.

### m6. The SHT40 is thermally tied to the backplate and wall (estimate)
- **Where:** a PETG standoff r 2.6, 11.5 tall (L543), plus two r 1.6 posts (L545), plus a steel M2 screw.
- **Why it matters:**
  - Rough conductance: about 7e-4 W/K through the supports, against about 1.3e-3 to 2.5e-3 W/K of convection off a 12.6 × 10.5 board.
  - So the sensor reads 20-35 % of the way from air temperature to backplate temperature.
  - On an exterior wall the backplate follows outdoor temperature. The resulting error changes with the weather, so a fixed calibration offset cannot remove it.
- **Fix:**
  - Use a hollow or thin-wall standoff (r 1.6 tube), drop one rest post, and use a nylon M2 screw.
  - Or hang the board from the cover, which faces room air.
  - Log the SHT40 against a reference thermometer over a cold night before trusting the offset.

### m7. The bottom M3 x 10 closure screw has zero thread margin
- **Where:** the screw head sits on the cover outer face at y = -57. The pilot ends at y = -47 (L564), so a 10 mm screw ends exactly at the pilot floor.
- **Why it matters:** any short pilot or tolerance error makes the screw bottom out and strip the PETG.
- **Fix:** extend the pilot and boss to y = -45 (boss L546: y to -43).

### m8. The XL7015 rail tie tunnels are about 1.0 mm tall with a 0.5 mm roof
- **Where:** L555-556. The notch cut z 2.4..4.0 in a rail spanning z 2.5..4.5, and the plate fills below z 3.
- **Why it matters:** standard 2.5 mm cable ties are 1.0-1.2 mm thick. They will not thread through, and the 2-layer roof breaks.
- **Fix:** cut the notch from the rail top down, as an open U to z 3.0, or make the tunnel 1.6 tall with a 1.0 roof.

### m9. XL7015 height and OLED frame (UNVERIFIED)
- **Where:** the OLED frame bottom is at z 20 (L468) over x 26..56, y 21.2..51.6, which covers the XL7015 (x 16.5..60.5, y 26.5..42.5). The parts budget above the XL PCB is 13.9 mm.
- **Why it matters:** the brief assumes 12 mm. A standing 100 V input electrolytic or a 3296 trimpot with its adjust screw can reach 13-14 mm, which would leave no margin.
- **Fix:** measure the board on arrival. If it is over 13 mm, lay the input cap flat, or drop the rails (L553-554) by 1 mm.

### m10. The field-cable strain relief is off the cable path
- **Where:** the zip-tie bridge is at x -22.5..-19.8 (L550). The window is x ±17, and the modelled field wires leave it at x -6..+6.5 going straight up (L369-376).
- **Why it matters:** an 18/8 thermostat cable (about 7 mm OD, stiff) has to bend left about 10-20 mm within z 3-8 to reach the tie, and its conductors then have to come back right. That is awkward but possible.
- **Fix:** move the bridge to the window's left edge (x ≈ -19, y -13.5), or model the jacket end and the actual bend. Cap the unused conductors (B and any spares).

### m11. The reference relay geometry clashes with the mounting holes
- **Where:** terminal and relay pitch is 17 mm (kx = -51, -34, -17, 0, L365, L798-801), so the 12-way span runs x -58.6..7.6. The bottom holes at (-59,-1.75) and (8,-1.75) (L41) fall inside the K1 and K4 terminal bodies.
- **Why it matters:** a real board cannot be built that way, so the field-wire landing x positions in the model are guesses.
- **Fix:** measure the terminal block positions and relay pitch, and update `kx`.

### m12. The SHT40 cable channel fit and the "slit" (UNVERIFIED)
- **Where:**
  - L122 says the cable is "pressed in through a slit from the top", but the code (L604) cuts only a closed 3.2 mm bore and no slit. L130 says "threaded before soldering"; pick one.
  - Four 26 AWG silicone wires (OD about 1.2-1.4 mm) need a 2.9-3.4 mm circle.
- **Fix:** make the bore 3.6 mm, or model the slit.

### m13. Countersunk #8 in PETG slots: creep and wedging
- **Where:** an 82° cone in a 4.4 mm slot (L194-205).
- **Why it matters:** the wedge loads the slot walls apart, and PETG creep loosens the clamp over months. Rattle is the likely result, not a fall.
- **Fix:** use #8 pan or washer-head screws on a flat seat (an 8.6 mm counterbore 1.5 deep instead of the cone), or add a TPU washer.

### m14. Document and code drift (the brief's claims against the files)
- **CLAUDE.md status:** "about 75 mm apart. The slots accept 65–85 mm". The code (L142-143) says measured 80, range 72–88.
- **HARDWARE_r1 UI:** "9-pin JST-PH". The brief says 4-pin + 6-pin inline. The code still has the 9-pin reference block (L783).
- **HARDWARE_r1 Sensor:**

  | HARDWARE_r1 says | Brief / code say |
  |---|---|
  | SHT40 about 18 × 12 | 12.56 × 10.5 |
  | It "stands" | It lies flat |
  | Seal the cable notch with putty | TPU grommet |

- **HARDWARE_r1 BOM:** "M3×4 heat-set inserts". The brief says no inserts.
- **L40 comment:** "75 x 55", against a measured 73 × 50.
- **`countersunk()` (L181-192):** a #6 helper that nothing calls.

### m15. The perfboard underside has only a 3 mm gap
- **Where:** bosses at z 1..6 (L541), so 3 mm under the perfboard.
- **Why it matters:** 18 AWG insulated links (OD about 2.0-2.3) plus solder joints of 1-1.5 mm do not fit underneath.
- **Fix:** run the 18 AWG R and C links on the component side.

---

## Checked and fine (not findings)
- **Relay tops:** 1.9 mm under the cover. Nothing in the cover hangs over the relay area.
- **Wall slots:** the horizontal-left plus vertical-right pair gives about ±2.5° of levelling at 80 mm, and the screw spacing range is 72–88.
- **Window:** centred between the screws (x 0, y -13.5).
- **Pin choice:** no relay pin is a strapping pin or a TX pin.
- **Power path:** C = GND is consistent through the power path. The 1N5819 blocks USB backfeed into the 5 V rail.
- **Bus voltage:** the bus stays under 63 V for the cap and 80 V for the XL7015 at high-line 24 VAC, about 46 V peak.
- **USB-C cutout:** aligned with the modelled XIAO USB (z 17.1..20.3 against a cutout of 15..22.5).
- **Chamber exhaust port:** it opens into the D-pad cavity (L494-495 cut through both skins at z 16-21). Buoyancy runs chamber to cavity, so this is acceptable.
- **Not reviewed in depth:** dpad_module.py, apart from the Dupont mis-mating note in M1.
