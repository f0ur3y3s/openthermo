# Critic review: openthermo controller board (round 4)

Scope: the current layout on the 30 × 70 mm, 10 × 24-hole perfboard (HARDWARE.md "Controller board", `fusion_case_v3.py`
`PB/CTL/FUSE_*/CAP_AX/XIAO_CY/PB_SUPPORTS/PB_PRESS`, `build_wiring()`, Reference list), how the board sits in the case,
and how cables reach it. Grid: hole (c, r) = (−58.39 + 2.54c, −46.41 + 2.54r). Board x −64.18..5.82, y −49.98..−19.98,
z 6.0..7.6. Geometry was checked with `critic_work/check.py` (box envelopes from the brief; cylinders checked by distance
where boxes report false corner hits).

## Root cause (one sentence)

The XIAO's D0–D6 header is on **grid row 0, the last row of the board**, so no D-row pin has a free outside hole. Every
connection to D0–D6 therefore goes underneath. That pushed the design to "every wire solders to a header tail
underneath". Nearly every Blocker and Major below follows from that.

---

## Findings

### BLOCKER

**B1. The underside does not fit.** About 35 conductors run in the 3.0 mm gap, and about 25 of them converge under the
XIAO's 17.8 × 21 mm footprint.
- **Cable conductors on header tails underneath: 19.** Relay 6 + cover 9 + SHT40 4. As round bundles they are Ø3.7
  (relay, `r_bundle(6)` = 1.83), Ø4.7 (cover, 2.36) and Ø3.0 (SHT40, 1.5), against a 3.0 mm gap. Tails are trimmed to
  2.0 mm, so a wire lying across a tail has 1.0 mm left. Two crossing 26 AWG wires stack to 2.6–2.8 mm.
- **Logic jumpers implied by the table (insulated, underside): about 10.**
  - IN4 pull-down (0,1) → D10 (3,6): ≈15 mm, crossing the pull-down field.
  - 1k (4,1) → D6 (6,0).
  - 3V3 cap GND (3,7) → (1,7).
  - 1N5819 cathode (0,9) → (0,7), hopping over the SDA hole (0,8).
  - Optional pull-ups: (0,8) → D4 (4,0) ≈22 mm, (6,8) → D5 (5,0) ≈21 mm, (3,8) → 3V3 (2,6).
  - **XL7015 5 V (col 23) → 1N5819 anode (4,9): ≈48 mm.**
  - **C/GND (23,8) → XIAO GND (1,6): ≈56 mm.**

  The last two are not in HARDWARE.md at all, but the circuit needs them: the XL7015 lands at col 23 and the XIAO is at
  cols 0–6. They run under the antenna keepout and under the fuse pins.
- **Crossings.** The relay IN wires enter at the top-left and must reach row 0 (the bottom edge). The cover wires also
  enter at the top-left and land on both rows. The SHT40 enters from the right at y ≈ −40. With 19 wires plus about
  10 jumpers converging there, I count at least 10 unavoidable crossings, each a 2.6–2.8 mm stack bearing on cut tails.
- **Result.** The board cannot seat on its pads. The cover pins (0.15 mm gap) then clamp the stack onto 0.64 mm cut
  tails, some of which are 24 VAC (see M1). As drawn, it is not buildable by hand.

### MAJOR

**M1. 24 VAC and bus copper sits directly above logic wires, separated only by insulation and under clamp.**
- The SHT40 cable runs "flat under the board" at y ≈ −40 from x 6 to the XIAO. On the way it passes:
  - the cap leads (20,1) and (20,3) (bus+/GND);
  - the F1/F2 pins (19,1), (19,5), (10,1) and (10,5) (R);
  - the R link along col 19;
  - the F1-out → 1N4007 link.
- The cover and relay wires cross the long 5 V and GND jumpers under the keepout.
- A nicked jacket on a cut tail puts R (≈34 V peak) or the bus (≤39 V) onto 3V3, SDA/SCL or a GPIO. That kills the XIAO
  and leaves the relay inputs undefined.
- On the top side the logic and power areas are separated by 3 columns. Underneath they are not separated at all.

**M2. A pre-fuse R pad sits next to a bus pad.**
- F1-in (19,1) is orthogonally adjacent to the 470 µF lead (20,1). Centre distance is 2.54 mm; the copper gap is about
  0.7–1.0 mm.
- The bare "R along col 19" link also passes (19,3), which is next to the other cap lead (20,3).
- A solder bridge there is either:
  - R straight onto the cap, bypassing F1 and the 1N4007, so the 63 V electrolytic sees AC and vents; or
  - R–C ahead of **both** fuses, a short protected only by the HVAC transformer.
- This is the one place on the board where a perfboard bridge is not fused.

**M3. Relay-input adjacency hazards are hidden under the XIAO.**
- **IN4 next to IN1 and D0, underside, under the XIAO.** The IN4 pull-down's D10-side lead (0,1) sits between D0 (0,0)
  and the IN1 node (1,1). A W–Y1 bridge (aux strips tied to the compressor) or IN4–UP bridge would be invisible once the
  XIAO is fitted.
- **D10 (IN4 = W) next to 3V3 on the header.** This is inherent to the current pin map: 3V3 (2,6) is beside D10 (3,6).
  One header bridge leaves W latched on at boot and through every reset. The pull-down cannot override it.
- **D3 (IN3 = O) next to D4 (SDA), which is pulled up to 3V3 (≈3.2 k).** A bridge holds IN3 at ≈2.5 V whenever D3
  floats (power-on, reset). That flips the reversing valve without the minimum-off rule ever seeing it.
- None of this is checked by any bring-up step.

**M4. Wires soldered to the female-header tails are unreliable and unserviceable.**
- The GND tail (1,6) would carry: the relay, cover and SHT40 GND wires, the long C link, the 100 nF lead bridge and the
  pull-down bus. That is 4–6 conductors lap-soldered to a 2 mm, 0.64 mm-square stub.
- None of these joints has strain relief. All are invisible with the board seated.
- Any rework means: cover off → board out of its cradle (every harness must have flip-over slack) → desolder under the
  header plastic.
- The relay cable deliberately has no connector, so its 6 joints are permanent and hidden.

**M5. The board is not retained with the cover off.**
- On the wall, gravity is −y and the board's z axis points at the user. Nothing holds it in z without the cover: the
  fences are plain walls with no lips, and the board has no screws.
- While R and C are pushed into the horizontal-entry terminal and screwed down, the board lifts on its harnesses.
- **Left side.** There is no fence. The rim's inner face is at x −65.25 against the board edge at −64.18, a 1.07 mm gap.
  The right fences have a 0.15 mm gap. So the board has 1.2 mm of x play, and pulling the USB-C plug drags it left.
- With the cover on, the pins leave 0.15 mm of z play. That is fine.

**M6. The fuse holders are mechanically marginal.**
- **Base.** Each 26.8 × 10 × 17 mm holder stands on two 0.64 mm header pins soldered into perfboard pads.
- **Fuse changes.** Pressing or twisting the cap during a fuse change (a stated with-cover-off task) levers 17 mm of
  height onto two pads. Pad lift is likely after a few cycles.
- **Pitch.** The legs are 23.0 mm apart against 22.86 on the grid. Solder the pins to the legs while the holder sits in
  the board, as a jig, or the pins will be preloaded.
- **Height.** If the pins extend the legs (rather than lying alongside them), the holder rides on solder fillets or
  header plastic. Installed height can then exceed 17.9 mm (z 25.5 max with the 1 mm pocket). `FUSE_H` = 17 is unverified.
- **Fix.** Bond the holder bases to the board with epoxy or thick CA. The bond is mechanical only: the pins still carry
  the current.

**M7. The 18 AWG COM wire through hole (10,9) is not realistic.**
- Solid 18 AWG is 1.02 mm against a ~1.0 mm plated hole; stranded 18 AWG will not go in at all.
- The hole also needs its own 10 mm underside link from (10,5), carrying up to ~1.6 A.
- **Fix.** Solder the 18 AWG straight onto the F2 holder's output leg (a thick tab) on the top side. The hole and the
  link both go away.

**M8. USB-C reach is marginal.**
- The receptacle face is at x ≈ −61.5 (CAD) or ≈ −62.5 (pin-centred XIAO). The cover's inner face is at −67 and its
  outer face at −69.
- The plug's overmold must therefore sink 4.5–5.5 mm into a 13 × 7.5 mm (y × z) cut to seat.
- Many USB-C cables have overmolds wider than 12.5 mm or thicker than 7.5 mm. Those will not latch.
- **Fix.** Enlarge the cut to about 15 × 9 mm, or state "slim-plug cable only" in the docs. (The board cannot move left:
  there is 1.07 mm to the rim.)

### MINOR

**m1. 470 µF overhang.** Mechanically and electrically it is acceptable: about 12.4 mm of the body rests on the board and
there is 1.34 mm to the divider rib.
- `CAP_AX` bends the leads 1.0 mm from the bung, which stresses the seal. Bend at about 1.5 mm; that still clears the rib
  by 0.84.
- Glue the body to the board.
- Under it there is only gutter, no board. Use that gutter for the SHT40 cable going up (P1-R2), not under the board.

**m2. XL7015.** HARDWARE.md says a 3-wire cable, but only 2 holes are given, (23,4) and (23,5). Also, a wire rising from
(23,4) at y −36.25 runs 0.08 mm off the cap's tangent plane.

**m3. XIAO position in CAD.** The CAD XIAO box (x −60..−39) is 1.27 mm offset from a pin-centred body (x −61.27..−40.27).
- Verify against the Seeed drawing.
- If the body really is pin-centred:
  - The antenna clearance figures in HARDWARE.md ("4 mm plastic, 6 mm clips") are really 5.3 and 7.3 mm.
  - The USB is 1 mm deeper inside the case.
  - The bottom-left press pin clears the XIAO corner by only 0.33 mm.

**m4. Antenna.** The fuse clips (R) are about 6–7 mm from the antenna end, the bottom of the 5–10 mm band. With these
parts there is nowhere better: the cap cannot move right because of the rib. See alternative P2.

**m5. 100 nF on XIAO 3V3.** It adds nothing, since the XIAO already decouples its LDO and module. It also costs a jumper
and puts a GND hole next to D10. Drop it. The 5 V one is optional too.

**m6. Optional I2C pull-ups as drawn.**
- (0,8) (SDA) sits between two 5 V holes, (0,7) and (0,9). One bridge puts 5 V on SDA, which kills both I2C parts.
- Each pull-up also needs a ≈21 mm jumper.
- Fit them on the SHT40 module instead, if it lacks them.

**m7. Support pads over unused holes.** Pads overlap the copper of unused holes (8,0), (9,0) (pad −37,−48.6), (5,9)
(pad −46,−21.4), and (14,9), (15,9) (pad −21.5,−21.4). Keep those holes empty, or the board rocks on a fillet.

**m8. Relay harness channel.** The Ø3.7 relay harness has 3.25 mm between the module edge and the rim, so it must lie
flat or over the module edge. The CAD path's start point, (−61, −18, 4.9), clips the top-left support pad by ≈0.2 mm.

**m9. Cover bundle over the wall screw.** The cover bundle at y −17.5 (Ø4.7) overlaps the left wall-screw head envelope
(y −17.8..−9.2) by about 2.7 mm. A driver needs the bundle pushed aside. Tie it on the +y side of the screw.

**m10. COM wire route.** The COM wire rises at x −33 to z 21. It is about 14 mm from the antenna, which is acceptable.
Run it at board level to y −20 before rising, to stay further away.

**m11. Pull-down spacing.** The pull-down bodies in cols 0–4 are 0.14 mm apart (Ø2.4 at 2.54 pitch). That is fine but
touching.

**m12. Build order (current).** The order is forced: female headers → pull-downs → the ~10 insulated underside jumpers →
the power parts and their 6 links → then 19 cable wires onto the tails, with the board out of the case and every cable
pre-cut. Any later fix is a de-install.

---

## Proposed layout P1-R2 (recommended)

The principles:
- **Move the XIAO up one row** so both header rows have outside holes.
- **Land every cable on top**, on male-header "posts" (pins you already have) next to its XIAO pin, joined by a 1-pitch
  underside bridge. The cable wires get strain relief, soldering from the front with the cover off, and a flat underside.
- **Two electrically separate islands on the board**, logic (cols 0–6) and power (cols 10–23). The only links between
  them are the XL7015 (common negative) and the relay module.
- **A small re-pin** (one edit in `board.h`, plus re-running the relay bench checks) so that **no relay IN pin is next to
  3V3, 5V, an I2C line or TX**.

The board does **not** move or rotate. The USB end must face the left wall, the field window sits above the right end
and the SHT40 grommet is at the right, so the current position and orientation are already right. It cannot go up
(grommet flange, wall screw), and down gains nothing.

### Re-pin (P1-R2)

| XIAO pin | GPIO | Now | P1-R2 |
|---|---|---|---|
| D0 | 0 | UP | **IN4 (W)** |
| D1 | 1 | IN1 (Y1) | IN1 (Y1) |
| D2 | 2 | IN2 (G) | IN2 (G) |
| D3 | 21 | IN3 (O) | IN3 (O) |
| D4 | 22 | SDA | **LEFT** |
| D5 | 23 | SCL | **DOWN** |
| D6 | 16 | RIGHT (1k) | RIGHT (1k) |
| D7 | 17 | OK | OK |
| D8 | 19 | DOWN | **SCL** |
| D9 | 20 | LEFT | **SDA** |
| D10 | 18 | IN4 (W) | **UP** |

- **No banned pins.** None of 0, 1, 2 or 21 is strapping, USB, UART TX or flash.
- **Neighbours of the four IN pins:** the board edge, each other, and D4 (a key with a weak internal pull-up). A bridge
  to a key gives ≈0.6 V, below the threshold.
- **Remaining IN–IN adjacency** is inherent to the header. Add an ohm test to bring-up (below).
- **To verify:** that GPIO0/1 have no 32 kHz crystal on the XIAO C6, and the GPIO0 reset state. Bring-up step 2.4
  already scopes the IN pins through a reset.

### Logic island (XIAO on rows 1 and 7, XIAO_CY = row 4 = −36.25; USB cut moves +2.54 in y)

| Item | Holes (col, row) | Notes |
|---|---|---|
| XIAO female headers | (0..6, 1) = D0..D6; (0..6, 7) = 5V GND 3V3 D10 D9 D8 D7 | Body y −45.15..−27.35 |
| Pull-down IN4/IN1/IN2/IN3 (10 k, lying) | col 0, 1, 2, 3: leads (c,2) and (c,6) | Under the XIAO, in the trough (2.4 tall vs 8.5). The top lead bends along the underside to (c,1) (the pin) and on to (c,0) (the post): the resistor's own lead is the bridge. |
| Pull-down GND bus | underside (0,6)–(1,6)–(2,6)–(3,6), plus (1,6)–(1,7) GND pin | 1-pitch bridges only. Neighbours are 5V/3V3/D10 (UP): a bridge there is a rail short or a stuck key, caught by the ohm test, not a relay hazard. |
| Bottom post strip (1×6 male header) | (0..5, 0): IN4, IN1, IN2, IN3, LEFT, DOWN | Each bridged to (c,1). Wires solder to the post tops from the front. Trim posts to z ≤ 13. |
| 1k, D6 (standing, used as a post) | lower lead (6,0), bridged (6,0)–(6,1) | The RIGHT wire lap-solders to its upper lead, under heat-shrink. Body y −47.6..−45.2, clear of the XIAO. |
| Top post strip (1×6) | (1..6, 8): GND, 3V3, UP, SDA, SCL, OK | Each bridged to (c,7). GND post: cover, SHT40 and XL OUT− wires (3). |
| 1N5819 (standing, used as a post) | cathode (0,9); bridges (0,9)–(0,8)–(0,7) to 5V | The anode lead is the **5 V rail post**: relay-cable 5V and XL7015 OUT+ wires. Body clears the strip corner by 0.46 mm and the XIAO (z ≤ 13.8 vs 16.1). |
| 2nd GND post | (1,9), bridged to (1,8) | Relay-cable GND. |
| 100 nF 3V3 (optional) / 5V | omit; if wanted, 3V3 across (1,9)–(2,9) (≤ 4 mm disc), with the relay GND moved to (1,8) | XIAO already decoupled. |
| I2C pull-ups | not on the board | Fit them on the SHT40 module if it has none. |
| Antenna keepout | cols 7–9, top **and underside**, empty | Only plastic: press pins at (−40.5, −21.4) and (−37, −48.6). |

**Logic underside:** zero insulated jumpers. About 22 one-pitch bridges, most of them resistor leads.

### Power island (positions mostly as now; fixes in bold)

| Item | Holes | Notes |
|---|---|---|
| R-C terminal | R (21,8), C (23,8); entries +y | unchanged |
| F1 T1A | in (19,1), out (10,1) | **Bond the base**; solder the pins to the legs in place |
| F2 T1.6A | in (19,5), out (10,5) | **COM 18 AWG soldered to the F2 out leg** on top, run at board level over (10,6..9) to y −20, then up. Hole (10,9) and its link are deleted. |
| 1N4007 | anode (11,8), cathode (14,8) | unchanged |
| 1.5KE51A | cathode (13,9), anode (19,9) | unchanged |
| 470 µF | **+ (20,4), − (20,2)** (moved up one row), body along +x over the edge, bend at 1.5 mm, glued | No pre-fuse R pad orthogonally next to a cap lead; the nearest is diagonal, 3.59 mm |
| XL7015 IN pair | **bus+ (22,6), GND (23,6)** | 2 wires; clear of the cap (2.3 mm) and the terminal (1.4 mm) |
| Underside links | R: **insulated** (21,8)→(19,5)→(19,1). F1-out (10,1)→(11,8) insulated. Bus (13,9)→(20,4)→(22,6) insulated. GND: bare spine (20,2)–(21,2)–(22,2)–(23,2), then col 23 up to (23,8); TVS anode (19,9)→(23,7) insulated | 6 insulated links, all ≤ 20 mm, all inside the power island, no logic wire under it. **Never bare on col 19.** |

### Cables (P1-R2)

| Cable | Route | Lands on |
|---|---|---|
| Relay (6 × 26) | Left channel → top-left corner | 5V → 1N5819 anode post (0,9). GND → (1,9). IN1..IN4 down the left margin (x −62..−60, z 8–12, under the USB overhang, below the plug at z ≥ 15) → posts (1,0), (2,0), (3,0), (0,0). No crossings. |
| Cover (9) + SHT40 (4) | One bundle along y −17.5, as now. The SHT40 now goes **up the right gutter** (x ≈ 10) from the grommet, under the cap overhang at z 3–6, and joins the cover bundle at (10, −17.5) instead of running under the board. | GND (1,8), 3V3 (2,8), UP (3,8), SDA (4,8), SCL (5,8), OK (6,8). LEFT, DOWN and RIGHT go down the left margin with the relay INs → (4,0), (5,0), 1k post (6,0). |
| XL7015 | Split: IN pair → power island (22,6)/(23,6) as now. **OUT pair (5V, GND) runs with the cover bundle** → 1N5819 anode post and (1,8). | Removes the 48 mm and 56 mm cross-board jumpers. Every 5 V/GND current stays star-fed from the logic island, so a broken relay-cable GND leaves the module without ground (relays off). Do **not** feed the relay module straight from the XL7015 while the XIAO gets its GND only through the relay cable: if that one GND wire breaks, the XIAO's ground floats to ≈5 V and its GPIO clamps can drive the IN pins high. |
| COM (18 AWG) | From the F2 out leg, at board level to the top edge, then up and over, as now | ≥ 13 mm from the antenna |
| Strain relief | Hot-glue or tie the relay and cover bundles at the board's top-left (a printed tie bridge like the field-wire one at about x −63, y −17) before the posts | |

### Mechanical changes for P1-R2

- `XIAO_CY = PB_Y0 + 4*P`. The USB cut follows. Enlarge it to about 15 × 9 mm (M8).
- `PB_PRESS`:
  - Drop (−62.5, −48.6) and (−46, −21.4): they sit on the wire fan and on the top-post wires.
  - Add (−40.5, −21.4) (keepout, plastic only) and a Ø2.0 pin at (−63.2, −44.5). That second pin is on the left margin,
    beside the bundle and outside the USB plug's y range (−42.75..−29.75).
  - Keep (−37, −48.6), (3.6, −48.6) and (−36.5, −21.4).
  - The overlap check found no conflict with the bodies.
- **Retention with the cover off (M5).** Drill two Ø2.2 holes in the hole-free end margins, at about (−61.5, −22.2) and
  (3.4, −47.8), and use M2 × 6 into enlarged pads (r 2.5). Alternatively, give the bottom and top fences 0.5 mm
  snap lips with a 45° lead-in, which prints without supports.
- **Left edge.** Add a pad or bump at x −64.33 on the left edge to take out the 1.07 mm rim gap.
- **Reference list.** Update the pull-downs (cols 0–3, rows 2–6), posts, the standing 1k and 1N5819, the cap (rows 2/4)
  and the XL holes. Delete the opt. pull-ups and the 100 nF caps.

### Build order (P1-R2)

1. Trough pull-downs, with their leads bent as the col 0–3 bridges and the row-6 GND bus.
2. Female headers on rows 1 and 7.
3. Post strips, the standing 1k and 1N5819, and the top-row bridges.
4. Power parts and their 6 insulated links. Bond the fuse holders and the cap.
5. **Ohm test** (new bring-up step 0):
   - each IN pin to each other IN pin, 3V3, 5V, SDA and SCL: **> 1 MΩ**;
   - each IN pin to GND: **10 k**;
   - R to bus+, GND and C: **open**;
   - 3V3 and 5V to GND: not shorted.
6. Bench power-stage test, with no XIAO.
7. Seat the board and fit the M2 screws.
8. Solder the cable wires to the posts from the front, with the XIAO unplugged (the posts are 1.26 mm from its edge).
9. Plug in the XIAO.

### Check result (`critic_work/check.py`)

- **Current layout:** no body overlaps (closest: 100 nF to the header 0.02, 1N5819 to the pull-up 0.04, F1 to F2 0.16).
- **P1-R2:** the only box hits are cylinder-against-box corner artefacts:
  - 1N5819 to the strip corner: real clearance 0.46 mm;
  - 1N5819 to the left-margin bundle: 0.04, so route the bundle at x ≤ −59.8;
  - 1N5819 to the optional 100 nF: 0.06, so use a ≤ 4 mm disc or omit it.

## Alternatives

- **P1, no firmware change.** Same XIAO rows 1/7, posts and islands. The extra cost:
  - IN4 (D10) gets a top post (3,8), bridged to (3,7).
  - Its pull-down is a standing hairpin from (3,9) [bridged to (3,8)] to (1,9) [bridged to (1,8) GND], with 5.08 lead
    spacing.
  - UP takes (0,0), SDA (4,0) and SCL (5,0); the relay IN1–3 posts are (1..3,0).
  - It keeps the inherent D10–3V3 and D3–SDA header adjacencies (M3), so the ohm test is mandatory.
- **P2, radio.** The XIAO C6 has a U.FL connector, and GPIO14 high selects it. A small 2.4 GHz FPC antenna on the inside
  of the cover's top wall removes the keepout and fuse-clip proximity issue entirely, for a one-line `board_init()` change.
- **P3, NPN drivers** (if bring-up needs them). Keep them on a strip at the relay module's input terminal, as HARDWARE.md
  already says. The 10 k pull-downs stay on the controller board.
