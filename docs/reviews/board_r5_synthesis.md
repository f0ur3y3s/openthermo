# Synthesis: openthermo controller board, round 5 (board only)

**Inputs:** BRIEF.md, HARDWARE.md, CLAUDE.md, r4_*, critic.md (+ JSONs, critic_work/), sympathizer.md (+ JSON, symp/).

**My tools:** one independent checker, in `review5/synth/`. Neither reviewer's code is used.

| File | What it does |
|---|---|
| `sgeo.py` | Geometry |
| `scheck.py` | The checker |
| `layouts_in.py` | r4, critic #1 and S3, re-transcribed into one format |
| `final_in.py` | The final layout and the re-pin option |
| `sens.py` | Tolerance sweep |
| `draw.py`, `make.py`, `export.py` | Drawings and JSON |

**What the checker models:**
- **Envelopes and clearances.** Real plan envelopes with heights. The lying cap is a cylinder, so a lead may come up beside it. The XIAO is pin-centred, with its header strips, a 5 mm antenna zone and the USB-plug lane. Part-to-part clearance is computed wherever the height ranges overlap. Anything under 0.3 mm is flagged.
- **Holes.** A lead or landing hole under another body is an error. A hole is assigned to every net whose bare underside link passes over it.
- **Pad adjacency.** Every **orthogonal and diagonal** pad pair is classified:
  - BLOCKER: R next to anything, or 24 VAC next to logic or the bus.
  - HIGH: bus next to logic.
  - RELAY-ON: an IN next to 3V3, 5V/5A, SDA, SCL, D6 or RIGHT.
  - MED: 5 V next to a GPIO.
  - Lower classes: IN–IN, bus–C, IN–key, supply–GND.
- **Wiring.** Net connectivity, link crossings, link count and pitches.
- **Distances.** 24 VAC to logic and to the antenna, plus antenna-at-edge and heights.
- **Tolerances.** `sens.py` re-runs the power island with a cap Ø10.5 (sleeved), fuse holders 10.2 wide, and terminal pins ±1 mm off-centre.


> **Amendment (Oct 8, 2026, user-measured fuse holder width 10.0 mm):** F2 moves from row 9 to **row 8**, side by side
> with F1 (pins 4 rows apart, 0.16 mm plastic gap). F2: R in (23,8), out (14,8); COM lands at (13,8); the R strap runs
> (23,0)…(23,8); (23,9), (13,9) and (14,9) are now keep-empty. This removes F2's 1.4 mm overhang past the top edge and
> one pitch of R strap. Re-checked with the same checker: no errors, no new hazard pairs, all distances unchanged.
> `final_layout.json` and `final_layout.png` reflect it; text below that still says "rows 4 and 9", "2.7 mm gap",
> "COM (13,9)" or "F2 1.4 over the top edge" is superseded.

## 1. Verdict

1. **Neither finalist builds as drawn.** Both power islands depend on plastic gaps that only exist at nominal size:
   - **Critic #1:** F1–terminal 0.06 (the critic did not flag this one), F1–F2 0.16, F1–cap 0.16. With a Ø10.5 cap or 10.2 mm holders, each of these becomes a collision.
   - **S3:** the standing TVS overlaps the F1 holder already at nominal size (−0.03). Cap–terminal is 0.06 and cap–F1 0.16.
2. **The critic's floor plan is the right one.** It has:
   - the XIAO in portrait, with the antenna 0.7 mm inside the top edge;
   - USB on the bottom edge;
   - a logic island that already has the minimum possible number of relay-on pad pairs for this pin map (2 header + 4 diagonal);
   - zero insulated links.

   The sympathizer's landscape layout puts the antenna 13.7 mm inside the board (rule 4 fails) and needs a 22.5 mm cap.
3. **The final layout:**
   - **Logic island:** the critic's, unchanged.
   - **Power island:** new. It takes the sympathizer's ideas: fuses at rows 4 and 9 with a 2.7 mm gap, one straight R strap down column 23, and the terminal at the bottom right with its wires entering from the bottom edge. Then:
     - the **470 µF lies** under F1 with its axis dropped by a lead dog-leg;
     - the **TVS stands** at the cap's lead end;
     - **D1** lies in column 12.
   - **Fit:** every power-part gap is at least 0.56 mm nominal and at least 0.21 mm in the worst tolerance stack. Nothing is taller than 17 mm.
   - **Wiring:** 25 underside links, 53 pitches, 0 insulated, 0 crossings.
   - **Safety:** R touches nothing, diagonals included. 24 VAC is 8.0 mm from the nearest logic pad and 14.0 mm from the antenna. There are no BLOCKER or HIGH pairs.
4. **No firmware re-pin by default.** The re-pin is a separate, costed option (§7). It is the only way to reach zero relay-on pairs.

### Same checker, all four boards

| | r4 | critic #1 | S3 | **FINAL** | re-pin option |
|---|---|---|---|---|---|
| Overlap / fit (< 0.3 mm, power parts) | ok | F1–term 0.06, F1–F2 0.16, F1–cap 0.16 | **TVS × F1 collision**, cap–term 0.06, cap–F1 0.16 | **none** (min 0.56; 0.21 at worst tolerance) | same as final |
| R next to another net (orth / diag) | 0 / **2** (R–bus) | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| 24 VAC or bus next to logic | 0 | 0 | 0 | 0 | 0 |
| Relay-on pairs (orth / diag) | 3 / 5 | 2 / 4 | 3 / 5 | 2 / 4 (the minimum for this pin map) | **0 / 0** |
| Bus–C pairs (blow F1) | 3 / 6 | 0 / 1 | 4 / 7 | 0 / 1 | 0 / 1 |
| 24 VAC pad → nearest logic pad | 10.5 | 7.6 | 15.5 | 8.0 | 8.0 |
| 24 VAC pad / plastic → antenna zone | 7.3 / 5.3 | 12.7 / 14.5 | 12.4 / 15.5 | 14.0 / 14.5 | same |
| Antenna end at an edge | no (13.7 in) | yes (0.7) | no (13.7) | yes (0.7) | yes |
| Links / pitches / insulated or sleeved | 31 / 80.5 / 5 | 30 / 50 / 0 | 26 / 61 / 0 | 25 / 53 / 0 | 25 / 53 / 0 |
| Crossings | 0 | 0 | 0 | 0 | 0 |
| Landings under the XIAO | 2 | 4 (+UP at the PCB edge) | 2 | 4 (+UP at the PCB edge) | 3 (+IN4 at the PCB edge) |
| Tallest | 17 | 17 | **22.5** | 17 | 17 |
| Standing parts | PD4, 1N5819 | TVS | PD4, 1N5819, cap, TVS | TVS | TVS |
| Overhangs (mm) | cap 7.8, F1 1.4 | F2 1.4 | F2 1.4, cap 1.4, terminal 0.4 | F2 1.4, cap 1.8 (lying, ~80 % supported), terminal 0.4 | same |

The "relay-on" rows count pads, not risk. Every one of these pairs is caught by the same net-level ohm test (§3). Fewer pairs means fewer chances to need rework, not more safety.

## 2. Rulings

### Critic

| Claim | Ruling | Basis |
|---|---|---|
| r4: pre-fuse R (19,5) is diagonal to the cap + (20,4) and to XL IN+ (20,6) | **CONFIRMED** | Checker: 2 diagonal BLOCKERs. r4's "no R pad next to anything" held only orthogonally. |
| r4: 3 orthogonal bus–GND pairs at (20,4/6/7)–(21,x) | **CONFIRMED** (low severity: blows F1) | 3 orthogonal + 6 diagonal |
| r4: 8 relay-on pairs; 1k under the antenna end; antenna not at an edge; 24 VAC 5.3 mm from the antenna | **CONFIRMED** | 3 orth + 5 diag; (6,2) and (6,6) inside the antenna footprint; 5.31 / 7.28 mm |
| Portrait XIAO is the only pose with the antenna at an edge and the USB usable | **CONFIRMED** | Landscape with the antenna outward needs a ≥ 16 mm lane under 7.5 mm through the middle of the board, so the USB points inward. |
| #1 power island fits ("fully on the board") | **REJECTED** | F1–terminal 0.06 was not flagged. F1–F2 0.16 and F1–cap 0.16. Every one becomes a collision with a Ø10.5 cap or 10.2 mm holders. |
| #1: 24 VAC 7.6 mm from logic; 12.7 mm from the antenna | **CONFIRMED** | F1o (12,6) to RIGHT (9,6). Final: 8.0 / 14.0. |
| #1: 4 landings under the XIAO | **CONFIRMED.** Accepted in the final | The minimum for this pin map is 2 (see §3). The 4 used here keep the whole relay bundle entering at one corner; they are landed with the XIAO out, from the open USB end. |
| #1: relay-on pairs 2 header + 4 diagonal | **CONFIRMED, and it is the minimum** | 3V3 and IN4 must each leave their pin through a hole next to the other pin, so 2 diagonals are forced. The same holds for IN3/SDA. |
| IN4–3V3 bridge: "3V3 collapses into a reset loop, W chatters" | **OVERSTATED** (mechanism) | The LDO out-drives a GPIO sinking current. The likelier outcome is W held on while the pin is overstressed. That is just as bad, and step 0 catches it. |
| IN3–SDA bridge: O on at boot, then I2C dies and the sensor fault clears it | **CONFIRMED** | |
| Re-pin: zero relay-on pairs | **CONFIRMED** | Checker: 0 / 0. The critic's "25 % better" is its own score and is not comparable. |
| C and logic GND join only through the XL7015; add C ↔ XIAO GND = 0 Ω to step 0 | **CONFIRMED, adopted** | |
| r4 / #1 step 0: "each IN to every other IN > 1 MΩ" | **REJECTED** (also in r4 and HARDWARE.md) | With 10 k pull-downs, IN-to-IN reads **20 kΩ**. The right test is IN→GND = 10.0 k each; 5 k means two INs are bridged. |

### Sympathizer

| Claim | Ruling | Basis |
|---|---|---|
| The r4 logic island is the best thing on the board; landscape wins | **OVERSTATED** | It is sound, but it fails rule 4 (antenna 13.7 mm inside). It also has 1 more orthogonal and 1 more diagonal relay-on pair than the portrait island. |
| S3: 24 VAC 11.5 mm from logic and the antenna | **CONFIRMED, and better than claimed** | Pads: 15.5 to logic, 12.4 to the antenna |
| S3: one straight R strap on column 23, 0 insulated links, fuses at rows 4/9 with a 2.7 mm gap | **CONFIRMED, adopted** | |
| S3 "fits with 0.03–0.16 mm gaps; glue cap, TVS and holder as a block" | **REJECTED** | At nominal size the TVS overlaps F1. With any real tolerance the cap hits F1 and the terminal. A glued block is not a build method. |
| S3's only cost is the 22.5 mm cap | **REJECTED** | It also has the fit failure, fails rule 4, and has 4 orth + 7 diag bus–C pairs (parallel bare straps on rows 1 and 2). |
| "A lying cap fits nowhere but over the right edge" | **REJECTED** | It lies under F1 with its axis 2.9 mm below the lead-hole midpoint: 0.56 to F1, 0.84 to the terminal, 1.8 overhanging the bottom edge (final). |
| Vertical fuses rejected | **CONFIRMED** | |
| S4 portrait loses (24 VAC 5.5, bus 3.3) | **CONFIRMED for S4 as built only** | This is not a property of portrait: the final is 8.0 / 7.6. |
| Option B re-pin (SCL→D6, RIGHT→D5, deletes the 1k) | **PLAUSIBLE, not adopted** | No START condition while SDA idles, so the ROM TX burst on SCL is harmless. It is a firmware change for tidiness only. |

### Brief-specific checks

- **Diagonals.** Done for all four boards (table above).
- **Antenna.** The final has 14.0 mm from 24 VAC metal to the antenna zone and 14.6 mm from any bus metal. No logic part sits under the antenna zone.
- **USB.** The face is 7.3 mm inside the bottom edge. Nothing taller than 7.5 mm is in the plug lane. The 5A landing (5,1) is inside the lane: dress that wire flat (it sits about 1.5 mm up; the plug starts about 7.8 mm up).

## 3. Final layout (default pin map)

Frame: hole (c,r) is at (2.54c, 2.54r) mm. The board is x −5.80..64.22, y −3.57..26.43, with z up from the board top.

Key: `*` = cable landing, `x` = keep empty. g = logic GND, C = power GND, B+ = bus, f1 = F1 out, CM = COM, R = unfused 24 VAC.

```
        0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23
r9    OK* OK   .   .   .   .   .  d6  d6  d6   .   .   x  CM* CM   x   .   .   .   .   .   .   x   R     <- top edge: XIAO antenna (cols 0-7), F2 +1.4
r8    Dn* Dn   .   .   .   .   .  SC  SC*  .   .   .   x   x   x   x   .   .   .   .   .   .   x   R
r7    Lf* Lf   .   .   .   .   x  SA  SA*  .   .   .   .   .   .   .   .   .   .   .   .   .   x   R
r6    i4* i4   x   g   .   .  i3* i3   x  Rt*  .   x   x   x   x   .   .   .   .   .   .   .   x   R
r5     x  3V  3V*  g   .   .  i2* i2   .   .   .   x  f1  f1   x   x   .   .   .   .   .   .   x   R
r4     g   g   g   g   .   .  i1* i1   .   x   x   x   x  f1  f1   x   .   .   .   .   .   .   x   R
r3     g  5V   .   .   .   .   .  Up   .   x  B+  B+*  x   x   x   x   .   .   .   .   .   .   x   R
r2     g  5V   .   .   .  5A   .  Up*  .   x   x  B+  B+   x   .   .   .   .   .   .   .   .   x   R
r1     g*  .   .   .   .  5A*  .   .   .   .   C   x   x   x   .   .   .   .   .   .   .   .   x   R
r0     g*  .   .   .   .   .   .   .   .   C*  C   C   C   C   C   C   C   C   C   C   C   C   x   R     <- bottom edge: USB, cap +1.8, terminal wires
```

Drawing: `final_layout.png`. Machine-readable: `final_layout.json` (`final`).

### Parts

Envelopes are in mm, as x × y × z.

| Part | Lead holes (net) | Orientation | Envelope |
|---|---|---|---|
| XIAO ESP32-C6 on 2 × 7 female headers | **col 7 rows 3–9:** D0 UP, D1 IN1, D2 IN2, D3 IN3, D4 SDA, D5 SCL, D6. **col 1 rows 3–9:** 5V, GND, 3V3, D10 IN4, D9 LEFT, D8 DOWN, D7 OK | Portrait. USB-C toward the bottom edge (face 7.3 mm inside it). Antenna end toward the top edge (0.69 mm inside it). Remove the black header spacer so the PCB sits 8.5 mm up. | PCB 1.26..19.06 × 4.74..25.74 × 8.5..13.2. Antenna zone y 20.74..25.74. Headers z 0..8.5. |
| PD1 10 k (IN1) | (6,4) IN1, (2,4) GND | Lying along x, under the XIAO | 7.0..13.3 × 9.0..11.4 × 0..2.6 |
| PD2 10 k (IN2) | (6,5) IN2, (3,5) GND | Lying, span 3 (body must be ≤ 6.5 long) | 8.3..14.6 × 11.5..13.9 × 0..2.6 |
| PD3 10 k (IN3) | (6,6) IN3, (3,6) GND | Lying, span 3 | 8.3..14.6 × 14.0..16.4 × 0..2.6 |
| PD4 10 k (IN4) | (0,6) IN4, (0,2) GND | Lying along y in column 0, against the header strip (plastic to plastic, fine) | −1.2..1.2 × 7.0..13.3 × 0..2.6 |
| 1N5819 | K (1,2) 5V, A (5,2) 5A | Lying on row 2, band at column 1 | 5.0..10.2 × 3.7..6.4 × 0..2.9 |
| 1 k (D6 → RIGHT) | (9,9) D6, (9,6) RIGHT | Lying along y in column 9 | 21.7..24.1 × 15.9..22.2 × 0..2.6 |
| **F1** T1A holder | in (23,4) R, out (14,4) F1o | Along x | 33.59..60.39 × 5.16..15.16 × 0..17 |
| **F2** T1.6A holder | in (23,9) R, out (14,9) COM | Along x | 33.59..60.39 × 17.86..27.86 × 0..17 (1.43 over the top edge) |
| **R–C terminal**, 5.08 | C (21,0), **R (23,0)** | Pins along x; wire entries face −y (bottom edge) | 50.78..60.98 × −4.0..4.0 × 0..12 (assumes the pins are centred in an 8 mm depth) |
| **470 µF 63 V** | − (11,0) C, **+ (11,2) BUS** | **Lying**, body toward +x. Bend both leads down 2.0 mm from the bung, with a ≈2.9 mm dog-leg toward +y, so the axis sits at y −0.40 (z 5). | 29.94..49.94 × −5.40..4.60 × 0..10.2. Overhangs the bottom edge 1.8 mm; about 80 % is on the board. |
| **1.5KE51A TVS** | A (10,1) C (body over this hole), **K (10,3) BUS** (sleeved hairpin) | **Standing**, band at the top. Drill both holes to 1.3 if the leads are over 0.95 mm. | Body 22.75..28.05 × −0.11..5.19 × 1..10.5; hairpin to z ≈12.5 |
| **1N4007** | A (12,5) F1o, K (12,2) BUS | Lying along y, band at (12,2) | 29.1..31.8 × 6.3..11.5 × 0..2.9 |

**Off the board** (as in r4): the 100 nF caps, the I2C pull-ups (fit on a module only if a probe shows none), and the NPN drivers (only if bring-up step 2 fails).

### Minimum clearances

| Pair | Nominal | Worst case |
|---|---|---|
| F1 – cap | 0.56 | 0.31 with a Ø10.5 cap; 0.21 with Ø10.5 cap + 10.2 mm holders. If needed, drop the cap axis further. |
| Cap – terminal | 0.84 | |
| F1 – terminal | 1.16 | 0.16 if the terminal pins sit 1 mm toward the wire side (measure) |
| Cap – D1 | 1.69 | |
| F1 – D1 | 1.76 | |
| Cap – TVS | 1.89 | |
| F1 – F2 | 2.70 | |

### Off-board landings

Every wire enters from the top. Two cores twisted into one hole means drilling it to 1.3. Holes marked † also carry a part lead, so drill those to 1.3 as well.

| Hole | Net | Wire | Leaves by |
|---|---|---|---|
| (0,0) | GND | cover GND + SHT40 GND | bottom-left corner |
| (0,1) | GND | relay GND + XL OUT− | left |
| (0,6)† | IN4 | relay IN4 | left |
| (0,7), (0,8), (0,9) | LEFT, DOWN, OK | cover keys | left |
| (2,5) | 3V3 | cover 3V3 + SHT40 3V3 | bottom edge, then up the trough under the XIAO (land with the XIAO out) |
| (5,1) | 5A | relay 5V + XL OUT+ | bottom (inside the USB lane: dress it flat) |
| (6,4)†, (6,5)†, (6,6)† | IN1, IN2, IN3 | relay IN1–IN3 | bottom edge, then up the trough from the USB end |
| (7,2) | UP | cover UP | bottom (0.3 mm under the PCB edge) |
| (8,7), (8,8) | SDA, SCL | cover + SHT40, twisted per hole | top / right side of the XIAO; dress away from the antenna |
| (9,6)† | RIGHT | cover RIGHT | top |
| (9,0) | C | XL7015 IN− | bottom |
| (11,3) | BUS | XL7015 IN+ | bottom, between the TVS and the cap's lead end |
| (13,9) | COM | 18 AWG to the relay commons (drill 1.3) | top |
| Terminal (23,0) / (21,0) | R / C | field R and C, 18 AWG solid | bottom-right, entries face down |

### Underside links

There are 25, all **bare** and flat, with **0 crossings** and **0 insulated**.

**Power:**

| Net | Holes | Material |
|---|---|---|
| R | (23,0)…(23,9), straight | 18 AWG, lapped on the terminal R tail and the F1/F2 input tails |
| C | (9,0)…(21,0), straight along row 0, under the cap | 18 AWG |
| C | (10,1)–(10,0) | TVS anode tail |
| BUS | (10,3)–(11,3)–(11,2)–(12,2) | 18 AWG or the TVS cathode tail |
| F1o | (14,4)–(13,4)–(13,5)–(12,5) | 18 AWG or the 1N4007 anode lead |
| COM | (13,9)–(14,9) | the COM wire's own end |

**Logic** (lead offcuts, 1–3 pitches):

| Net | Holes |
|---|---|
| IN1, IN2, IN3 | (7,r)–(6,r) for r = 4, 5, 6 |
| IN4 | (1,6)–(0,6) |
| GND | (1,4)–(2,4)–(3,4)–(3,5)–(3,6) |
| GND | (1,4)–(0,4)–(0,3)–(0,2)–(0,1)–(0,0) |
| 5V | (1,3)–(1,2) |
| 5A | (5,2)–(5,1) |
| 3V3 | (1,5)–(2,5) |
| SDA | (7,7)–(8,7) |
| SCL | (7,8)–(8,8) |
| D6 | (7,9)–(8,9)–(9,9) |
| UP | (7,3)–(7,2) |
| LEFT, DOWN, OK | (1,r)–(0,r) for r = 7, 8, 9 |

### Keep empty: never solder or bridge

- **Next to R:** all of column 22.
- **Around the bus, F1o and COM:**
  - columns 9–11: (9,2), (9,3), (9,4), (10,2), (10,4), (11,1), (11,4), (11,5), (11,6);
  - column 12: (12,1), (12,3), (12,4), (12,6), (12,8), (12,9);
  - column 13: (13,1), (13,2), (13,3), (13,6), (13,8);
  - column 14: (14,3), (14,5), (14,6), (14,8);
  - column 15: (15,3), (15,4), (15,5), (15,8), (15,9).
- **Holes between an IN and a boot-high net:** (0,5), (2,6), (6,7), (8,6).

### Adjacency hazards and the step-0 checks that catch them

**No R, 24 VAC or bus pad touches logic, orthogonally or diagonally.**

| Pair (one bridge) | Effect | Caught by (fuses out, XIAO out, cables free) |
|---|---|---|
| IN4–3V3: (1,5)–(1,6) header, plus diagonals (0,6)–(1,5) and (1,6)–(2,5) | W on at boot, GPIO overstressed | IN4 → 3V3 open |
| IN3–SDA: (7,6)–(7,7) header, plus diagonals (6,6)–(7,7) and (7,6)–(8,7) | O on at boot | IN3 → SDA open |
| IN1–IN2–IN3: header, plus the trough holes (6,4..6) | Cross-drive | **Each IN → GND = 10.0 kΩ.** 5 k means two INs are bridged. |
| IN1–UP, IN4–LEFT | Key line at about 0.6 V | IN → key line open |
| Bus–C, diagonal (10,1)–(11,2) | F1 blows (safe) | Bus → C charges, not 0. C → bus on diode test reads about 0.7 V (TVS forward, which also proves polarity). |
| Supply–GND (5V/3V3 next to GND) | Rail short | 3V3, 5V, 5A → GND open; 5A → 5V reads about 0.2 V on diode test (1N5819) |

**The full step 0:**
- R → C, bus, F1o, COM and every logic net: open.
- F1o → bus: about 0.5 V forward on diode test (1N4007), OL in reverse.
- COM → everything: open.
- Bus → every logic net: open.
- After the XL7015 is connected: C → XIAO GND = 0 Ω.

## 4. Build order

1. **Measure** (§5).
2. **Drill to 1.3 mm:**
   - (13,9) for COM;
   - the shared holes (0,6), (6,4), (6,5), (6,6) and (9,6);
   - (10,1) and (10,3) if the TVS leads are over 0.95 mm;
   - (21,0) and (23,0) if the terminal pins bind.
3. **Fuse holders.** Solder the header pins to the clip legs with each holder seated at (14,4)/(23,4) and (14,9)/(23,9) as the jig. Tug-test, check ≤ 0.05 Ω, then lift them out.
4. **Power straps on the bare underside:** R down column 23, then C along row 0. Both are straight; nothing crosses them later.
5. **Terminal** (entries down), then **F1 and F2**. Solder them to the straps and epoxy the holder bases.
6. **1N4007,** then the F1o link.
7. **TVS standing:** anode body over (10,1), sleeved cathode hairpin into (10,3). Then the TVS-to-C link and the bus link.
8. **470 µF last on the power side.** Check the polarity twice: **+ goes in (11,2)**. Bend the dog-leg so the can rests on the board with ≥ 0.3 mm to the F1 base. Optionally add a dab of RTV.
9. **Partial step 0** on the power side. Then bench step 1: land the XL7015 IN pair at (11,3)/(9,0) and set 5.00 V with no logic fitted.
10. **Logic parts, all lying:**
    1. PD1–PD3, with their signal leads ending in (6,4..6), clear of the header holes;
    2. PD4 in column 0;
    3. the 1N5819 on row 2;
    4. the 1 k in column 9;
    5. the GND, 5V and 5A bridges.
11. **Female headers** on columns 1 and 7, rows 3–9. Use the XIAO as the jig. Then the 1-pitch bridges to the pins.
12. **Full step 0** (§3).
13. **Land the cables, XIAO out:**
    1. the trough wires first (IN1–IN3 and 3V3, entering from the USB end, flat and under 7 mm);
    2. then the left, bottom and right landings;
    3. then COM (bridge (13,9)–(14,9)) and the field wires.
14. Re-run the IN and R lines of step 0, then fit the XIAO.

## 5. Measure first

1. **Fuse-holder base width and installed height.** The width is uncritical now (a 2.7 mm gap); the height must be ≤ 17.9.
2. **Cap diameter over the sleeve.** If it is over 10.5, drop the axis further. Each extra 0.25 mm of radius costs 0.25 mm more overhang.
3. **Terminal pin line to back face.** It must be ≤ 4.8 mm to keep ≥ 0.3 mm to F1. Also measure the pin diameter.
4. **TVS lead diameter.** It decides whether (10,1)/(10,3) need drilling.
5. **1/4 W body length.** PD2 and PD3 sit on a 3-pitch span and need a body ≤ 6.5 mm. Otherwise move their GND ends to (2,5)/(2,6) and re-check.
6. **XIAO USB face to the D0 pin centre.** This confirms the pin-centred assumption behind the 0.69 mm antenna inset.

## 6. Decisions for the user

1. **Adopt the final** (critic logic island + new power island). It needs no firmware change.
2. **Accept the 470 µF lying 1.8 mm over the bottom edge.** The alternative is S3's standing 22.5 mm cap, which also has no fit margin. I recommend the lying cap.
3. **Re-pin (optional, §7).** It is the only way to reach zero relay-on pads. It costs a firmware edit and the full relay re-validation.
4. **Antenna fallback** if Thread RSSI is poor: the U.FL port with an FPC antenna on the cover (GPIO14).
5. **Fix the HARDWARE.md step-0 line** "IN to other IN > 1 MΩ". It is physically wrong (the reading is 20 kΩ). Use IN → GND = 10 k.

## 7. Re-pin option

Drawing: `final_layout_repin.png`. JSON: `repin_option`.

**What it buys:** relay-on pairs go from 2 orthogonal + 4 diagonal to **0 / 0**. The IN neighbours are only other INs, a key line (LEFT) and GND. Landings under the XIAO drop to 3; IN4 lands at (7,2) at the PCB edge.

**Firmware (`board.h`):**

| Signal | Now | After |
|---|---|---|
| IN4 | D10 / GPIO18 | **D0 / GPIO0** |
| UP | D0 / GPIO0 | D10 / GPIO18 |
| SDA | D4 / GPIO22 | D9 / GPIO20 |
| SCL | D5 / GPIO23 | D8 / GPIO19 |
| LEFT | D9 / GPIO20 | D4 / GPIO22 |
| DOWN | D8 / GPIO19 | D5 / GPIO23 |

**Other firmware and test work:**
- the relay pin set in `tools/check_safety_sources.py`;
- the `relay_boot_safe` bootloader hook, which must drive GPIO0 low;
- **redo every relay bench check** (reset, panic, bootloader drop);
- scope IN4 (GPIO0, an LP/XTAL_32K pin) through power-on, reset and flash: it must stay below 1 V;
- confirm there is no 32 kHz crystal on GPIO0/1.

A 4-pin variant (IN4↔UP, SDA→D9, LEFT→D4) gives the same zero, but it splits SDA and SCL onto opposite sides of the XIAO.

**Board changes:** the power island is identical. The logic island changes:

| Item | Change |
|---|---|
| Pull-downs | PD4, PD1, PD2, PD3 in the trough at (6,r)→(2,r), r = 3..6; GND bus down column 2 |
| 1N5819 | K (1,1), A (5,1), with 5V (1,3)–(1,1) |
| Left column | (0,2) relay GND + XL OUT−, (0,4) cover GND, (0,5) 3V3, (0,6) UP, (0,7) SDA, (0,8) SCL, (0,9) OK |
| Right side | (8,7) LEFT, (8,8) DOWN, (9,6) RIGHT |
| Relay | IN4 at (7,2); IN1–IN3 in the trough at (6,4..6) |
| 5A | (5,0) |

The relay-cable order at the board changes too.
