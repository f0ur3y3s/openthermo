# Sympathizer report: openthermo controller board, round 5 (board only)

Inputs: `BRIEF.md`, `HARDWARE.md`, `r4_layout.json`, `r4_layout.png`, `r4_synthesis.md`, `CLAUDE.md`.
Work is in `scratchpad/review5/symp/`:

| File | What it does |
|---|---|
| `geo.py` | Grid, real part envelopes and lead holes for every part in every mount/rotation, XIAO in 4 orientations, pad-pair hazard classes |
| `score.py` | Underside router (no two nets may share a hole, so routes cannot cross), landing picker, metrics, score |
| `search.py` | Stage 1: 64 XIAO poses x power-island beam x logic-part beam x routing orders |
| `search2.py`, `islands.py` | Stage 2: the round-4 logic island held fixed, power island beam-searched and auto-routed |
| `layouts.py`, `check.py`, `describe.py` | Hand-finished layouts (r4, S1–S4) as full specs, strict checker (overlaps, lead clearance, connectivity, crossings, required landings), R/AC neighbour audit, keep-empty list |
| `S3.png`, `S2.png`, `S4.png`, `r4.png` | Plots of each layout from the same model |

Frame: hole (c,r) at (2.54c, 2.54r) mm; the board is x −5.80..64.22, y −3.60..26.46. The round-4 case frame is local + (−58.39, −46.41).

## 1. Bottom line

1. **The round-4 floor plan is not a case artefact.** With the case removed, I searched all four XIAO orientations
   and every fuse, terminal, cap, TVS and diode rotation, then hand-finished the best portrait and landscape
   candidates. Landscape at the left end, USB on the short edge, still wins (369 against 412), and it keeps round 4's
   logic island exactly. The logic island (rows 1/7, landings on rows 0 and
   8–9, pull-down leads used as bridges) is the best thing on the board, and I keep it unchanged.
2. **The power island can be much better.** **S3, my best layout,** stacks both fuse holders along x at rows 4 and 9,
   with their inputs on the right edge. One straight bare R strap down column 23 then feeds both fuses, and C can
   never be enclosed. The 470 µF stands at the bottom edge.

   | Measure | Round 4 | S3 |
   |---|---|---|
   | 24 VAC metal to logic | 5.3 mm | 11.5 mm |
   | 24 VAC metal to antenna | 5.3 mm | 11.5 mm |
   | Insulated or sleeved links | 6 | 0 |
   | Underside link length | 77.5 pitches | 61 pitches |
   | Cap overhang | 7.8 mm | none (1.4 mm of the standing base) |
   | Crossings, R-neighbour violations, new relay-on pairs | 0 | 0 |

   The one cost is a **22.5 mm tall cap**, 5.5 mm over the soft 17 mm guide.
3. **If every part must stay at or under 17 mm, use S2.** It is round 4 with the whole power island moved one column
   away from the antenna: 24 VAC is 7.85 mm from the logic and the antenna (round 4: 5.3). The cost is a cap
   overhang of 10.3 mm (round 4: 7.8). I could not fit a lying cap, two non-touching fuse holders and the terminal
   with more antenna clearance than that.
4. **Turning the XIAO portrait (S4) puts both the antenna and the USB on board edges, but it loses overall.** I built
   it in full (same power island as S3). 24 VAC comes 5.5 mm from the logic, the bus 3.3 mm, and it adds a relay-on
   pad pair (3V3 next to IN4). The inner header row's six landings also need wires dressed along the XIAO.
5. **No re-pin is needed.** Round 4's safety re-pin stays a valid option. I add a cheaper option that moves no relay
   pin: SCL→D6 and RIGHT→D5. It deletes the 1 k resistor and the RIGHT landing under the XIAO.

## 2. Steelman of the round-4 layout as a board (quantified with the same checker)

| Decision | What it buys, measured | Survives the redesign? |
|---|---|---|
| **Zoning: logic (cols 0–6) / empty gap (cols 7–9, 30 holes) / power (cols 10–23)** | 0 on-board conductors cross the gap: logic GND meets C only off-board, through the XL7015 and the relay module. Closest AC pad to logic pad is 10.47 mm; closest bus pad to logic pad is 17.96 mm. | **Yes, kept.** S3 widens the gap to cols 7–10 (40 empty holes). |
| **XIAO landscape, rows 1/7, USB on the left short edge** | 21 outside holes for landings (row 0 plus rows 8–9). The USB plug path is clear (no part in the plug corridor). The trough under the XIAO (35 holes) holds every low part. | **Yes.** Stage 1's crude logic placer preferred portrait D (364 against 415), but hand-finished, landscape wins (S3 369, S4 412). |
| **Every cable wire lands on top, beside its pin** | 15 logic landings: 14 are one pitch from their pin or series part, one (cover GND (2,8)) is two. 0 cable conductors on the underside. Round 4's lane L, fans and trough wiring follow from this and, without the case, simply exit the nearest long edge. | **Yes, kept verbatim.** |
| **Pull-downs use their own signal leads as the bridge** | One lead joins pin, landing and pull-down for IN1–IN3: 3 bridges and 3 joints saved. The pull-down is physically between the relay wire and the pin, so it cannot be missed. | **Yes.** |
| **Planar power links** | 0 crossings (checked in plan view). Bare runs are 1-pitch bridges or straight straps. | **Yes, improved.** S3 needs no insulated jumper at all. Round 4 needed one only because its terminal sat between the two fuse inputs. |
| **Keep-empty pads** | Audit: 11 R pads, 0 of which touch a non-R pad. F1o and COM touch nothing but their own net. Only bus–C pairs (which blow F1) and logic pairs remain. | **Yes.** The same audit is applied to S2, S3 and S4, all clean. |
| **Step-0 ohm test** | Catches all 5 IN-adjacent pad pairs the layout adds (IN1–IN2 and IN2–IN3 twice, IN3–SDA once). Also catches the 4 inherent header pairs: D1–D2, D2–D3, D3–D4, D10–3V3. | **Yes.** It is mandatory in every layout here. |
| **Cap lying with a 1.2 mm bend, overhanging the right edge** | Keeps everything at or under 17 mm. The case was not the reason: on this board a lying 10 × 21 mm cap fits nowhere else beside two 26.8 × 10 holders (§3). | **Kept in S2.** Replaced by a standing cap in S3. |

Two weaknesses are not caused by the case:

- The antenna faces the fuse clips: 5.3 mm to the holder plastic and 7.3 mm to the clips.
- R reaches the fuses through an 11-pitch insulated run, because C sits between the terminal and the fuse inputs.

A small finding: the round-4 1N4007 cathode hole (14,8) is 0.33 mm in plan from the end of the lying TVS. That is fine, because a lying Ø5.3 body is only 1.9–3.4 mm high at that offset, but it is flagged by my 0.5 mm lead-clearance rule.

## 3. What the systematic search found

**Model:**
- Real rectangular envelopes with heights.
- Low parts (≤ 7.5 mm) may sit under the XIAO PCB, never under the header strips.
- A lead or landing hole must be ≥ 0.5 mm outside any other body.
- Fuse holders closer than 1.0 mm to each other are rejected.
- Bare underside links run hole to hole; a hole holds one net, so bare links cannot cross.
- Landings must be top-accessible and are charged by their path to an edge.

**Candidates:**
- Fuses: 348 placements (x or y, both polarities).
- Terminal: 1,648 (4 faces).
- Cap: 2,024 (standing, or lying in 4 directions).
- TVS: 2,820 (lying with a 5–6 pitch span, or standing).
- 1N4007, 1N5819, the pull-downs and the 1 k: 3,228 each (lying with a 3–4 pitch span, or standing).
- XIAO: 64 poses (4 orientations × centre col 3–6 × centre row 3–6).

**Score** (lower is better):
- 10/mm of 24 VAC–logic separation under 15 mm, plus 3/mm of bus–logic separation under 10 mm.
- 8/mm under 12 mm of antenna-to-power clearance, plus 4/mm under 15 mm of antenna-to-AC clearance.
- 0.5/mm of antenna end further than 4 mm from an edge.
- 25 per part blocking the USB plug.
- Links: 0.6 per bridge, 2 per run, 1 per pitch, 12 per insulated jumper.
- 3 × the hazard weight: R next to another net 15, AC next to logic or bus 30/15, IN next to a pulled-up net 6, IN–IN 3.
- Landing path cost, plus 3 per extra exit edge per cable.
- Overhang 2/mm, height 8/mm over 17.5.

### 3.1 XIAO orientation (stage 1, with the legal-island rules)

The logic parts were placed automatically here, so absolute scores are pessimistic. Compare the rows with each other.

| Orientation | Best score | What happened |
|---|---|---|
| **L** (USB at the left short edge, antenna into the board; round 4) | 415 | Good logic island. The beam's power islands mostly used vertical fuses, which trap C and fail to route. The hand-finished version is S3. |
| **R** (antenna at the left short edge, USB inward) | 423 | The USB plug corridor (22 mm × 12.4 mm at z 7.8–14.4) is blocked by power parts in every legal island (USB penalty 62). **Rejected:** no in-situ USB. |
| **U** (portrait, antenna at the bottom edge) | 405 | Needs a standing cap. The inner header row's landings run along the XIAO. |
| **D** (portrait, antenna at the top edge) | 364 | The best portrait. Hand-finished as S4 below, it scores 412 against 369 for S3. |
| Centre col 5–6, any orientation | none | No legal power island: two non-touching holders, the terminal and the cap don't fit. The XIAO must be at the very end. |

### 3.2 Power-island families (stage 2, round-4 logic island fixed; 3 part orders, beam 200, auto-routed)

| Family | Best d(24 VAC, logic) | Why it loses or wins |
|---|---|---|
| **Vertical fuses (along y, rows 0 and 9) as an "AC wall"** | 15.0 mm | Each vertical holder makes 5 columns unusable for leads (its body is ±1.97 pitches wide). There is no free hole next to F-out for COM or F1o, the terminal ends up between the two inputs (R must encircle C), and the cap is pushed into the antenna gap (4.8 mm). **Rejected.** |
| **Fuses along x at rows 0/8, cap and terminal in the 10.3 mm band between them** | 11.5 mm | The terminal must sit between the two fuse inputs, so R needs 2 insulated jumpers around C. **Rejected.** |
| **Round-4 family: fuses along x at rows 0/5, terminal top-right, cap lying over the right edge** | 5.3–7.85 mm | The only legal home for a lying cap. Antenna clearance is capped at 7.85 mm by the cap overhang (S2). |
| **Fuses along x at rows 4/9, inputs on the right edge, terminal bottom-right, cap standing** | 11.5 mm | R is one straight strap down col 23, C sits outside R, and nothing overhangs. **Best: S3.** |

**Why the lying cap forces the trade-off:**
- The board is 30 mm high. Two 10 mm holders with a legal 2.7 mm gap leave a band of at most 8.76 mm.
- A lying Ø10 cap needs a 10 mm band with its axis on a grid row.
- So the lying cap can only hang off the right short edge, and the fuses must end where the cap's leads begin. That
  limits the fuses to cols 10–19 (round 4) or 11–20 (S2, 10.3 mm overhang).
- Standing the cap is what frees the right edge for a clean R strap.

## 4. The best two layouts

| | r4 | S1 = r4 + tidy | **S3 (best)** | **S2 (best ≤ 17 mm)** | S4 portrait |
|---|---|---|---|---|---|
| Score (lower is better) | 539 | 539 | **369** | 477 | 412 |
| 24 VAC to logic (mm) | 5.31 | 5.31 | **11.46** | 7.85 | 5.52 |
| Bus to logic (mm) | 11.0 | 11.0 | 8.92 | 11.0 | 3.28 |
| Antenna to 24 VAC / to any power metal (mm) | 5.3 / 5.3 | 5.3 / 5.3 | **11.5 / 8.9** | 7.85 / 7.85 | 10.6 / 10.6 |
| Antenna end to its edge (mm) | 46 | 46 | 46 | 46 | **5.8** |
| USB end to its edge (mm) | 2.9 | 2.9 | 2.9 | 2.9 | 3.3 |
| Bridges / runs / insulated or sleeved | 14 / 11 / 6 | 14 / 11 / 6 | 11 / 15 / **0** | 13 / 12 / 5 | 13 / 15 / 0 |
| Underside length (pitches) | 77.5 | 77.5 | **61** | 79.4 | 61 |
| Crossings | 0 | 0 | 0 | 0 | 0 |
| Relay-on or IN–IN pad pairs added | 5 | 5 | 5 | 5 | 6 |
| R pads touching another net | 0 | 0 | 0 | 0 | 0 |
| Overhang (mm) | cap 7.8, F1 1.4 | same | **cap base 1.4, F2 1.4, terminal 0.4** | cap 10.3, F1 1.4 | as S3 |
| Tallest part (mm) | 17 | 17 | **22.5 (cap)** | 17 | 22.5 |

**Why S3:**
- It improves rules 1–3:
  - 24 VAC is 2.2× further from the logic and the antenna;
  - every R pad is in one straight column with an empty column beside it;
  - there are no insulated jumpers;
  - the current path is short: R strap → fuse inputs on the same edge → F1 out → 1N4007 at col 12 → bus along row 0 → cap.
- It keeps every round-4 logic decision.
- It pays only on rule 6, which is the lowest priority and soft.

**Why S2 is the runner-up:** it is the best layout that keeps the ≤ 17 mm envelope. It is a low-risk edit of round 4: shift the power island one column right, and rehome the terminal to (21,9)/(23,9) and the 1N4007 (standing). The cost is a larger cap overhang.

S1 is round 4 with two trough tidy-ups: straight PD1 (1,2)-(1,6) and PD2 (2,2)-(2,5) instead of the touching diagonals, and the 1 k moved to (5,2)-(5,5), off the antenna footprint. These tidy-ups are included in both S2 and S3.

## 5. S3 in full

![S3](symp/S3.png)

```
      0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
r9   5A 5a* G  i4  .  .  .  .  .  .  . cm* CM CM  .  .  .  .  .  .  .  .  R        F2: pins (14,9) out, (23,9) in
r8   5V g* g* i4* lf* dn* ok* .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  R
r7   5V G  3V i4 Lf Dn OK  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  R   <- XIAO row 7
r6    . G  3v* G  . rt* .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  R
r5    . G  G  G   . Rt  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  R
r4    .  .  .  .  .  .  .  .  .  .  .  .  f1 f1 f1 .  .  .  .  .  .  .  .  R        F1: pins (14,4) out, (23,4) in
r3    .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  R
r2    . i1 i2 i3  . d6 d6  .  .  .  .  c* C  C  C  C  C  C  C  C  .  .  .  R
r1   up i1 i2 i3 SA SC d6  .  .  .  .  b+* B+ B+ .  C  .  .  .  C  .  .  .  R   <- XIAO row 1
r0   up* i1* i2* i3* sa* sc* . . .  .  .  .  .  B+ B+ B+ B+ B+ .  C  C  C  .  R
```

`*` marks a cable landing (wire enters from the top). Columns 7–10 are completely empty: the antenna gap.

### 5.1 Parts

Envelopes are board-local mm, z above the board.

| Part | Lead holes (col,row) | Orientation | Envelope x / y / z |
|---|---|---|---|
| XIAO ESP32-C6 on female headers | row A (D0..D6) at (0..6, 1); row B (5V GND 3V3 D10 D9 D8 D7) at (0..6, 7) | Landscape, USB toward −x (left short edge), antenna toward +x | PCB −2.88..18.12 / 1.26..19.06 / 8.5–12.8; antenna zone x 13.12–18.12 |
| PD1 10 k (IN1) | sig (1,2), gnd (1,6) | Lying along y, span 4, under the XIAO | 1.34..3.74 / 7.01..13.31 / 0–2.7 |
| PD2 10 k (IN2) | sig (2,2), gnd (2,5) | Lying along y, span 3 | 3.88..6.28 / 5.74..12.04 / 0–2.7 |
| PD3 10 k (IN3) | sig (3,2), gnd (3,6) | Lying along y, span 4 | 6.42..8.82 / 7.01..13.31 / 0–2.7 |
| 1 k (D6 → RIGHT) | (5,2) D6, (5,5) RIGHT | Lying along y, span 3 | 11.5..13.9 / 5.74..12.04 / 0–2.7 |
| PD4 10 k (IN4) | sig (3,9), gnd (2,9) | Standing: body over (3,9), hairpin to (2,9) | ≈4.6..8.8 / 21.7..24.1 / 0–10.5 |
| 1N5819 | A (0,9), K (0,8) | Standing: body over (0,9), hairpin to (0,8) | −1.35..1.35 / 19.8..24.2 / 0–9 |
| F1 T1A (power) | in (23,4), out (14,4) | Along x | 33.59..60.39 / 5.16..15.16 / 0–17 |
| F2 T1.6A (COM) | in (23,9), out (14,9) | Along x, 1.4 mm over the top edge | 33.59..60.39 / 17.86..27.86 / 0–17 (2.7 mm gap to F1) |
| R-C terminal 5.08 | R (23,0), C (21,0) | Pins along x, **wire entries face −y (bottom edge)** | 50.78..60.98 / −4.0..4.0 / 0–12 |
| 470 µF 63 V | + (17,0), − (19,0) | **Standing**, centred on (18,0); 1.4 mm of its base past the bottom edge | 40.72..50.72 / −5..5 / 0–22.5 |
| 1N4007 | A (12,4), K (12,1) | Lying along y, span 3 | 29.13..31.83 / 3.75..8.95 / 0–3 |
| 1.5KE51A TVS | A (15,1), K (13,1) | Standing: body over (15,1) (anode), cathode hairpin 2 pitches to (13,1) | 35.45..40.75 / −0.11..5.19 / 0–13.5 (touches the F1 holder and the cap sleeve; glue the three as a block) |

**Not on the board:** the 100 nF parts (XIAO and XL7015 decouple themselves), the I2C pull-ups (on the module, only if a probe shows none), and the NPN drivers (only if bring-up step 2 fails). This is the same as round 4.

### 5.2 Off-board connections (all land on top)

| Hole | Wire | Leaves by |
|---|---|---|
| (0,0) | Cover UP | Bottom edge |
| (1,0), (2,0), (3,0) | Relay IN1, IN2, IN3 | Bottom edge |
| (4,0), (5,0) | SDA, SCL (cover + SHT40 twisted per hole) | Bottom edge |
| (2,6) | 3V3, cover + SHT40 | Under the XIAO, entering from the left end |
| (5,6) | Cover RIGHT | Under the XIAO, entering from the left end |
| (1,8) | Relay GND + XL OUT− | Top edge |
| (2,8) | Cover GND + SHT40 GND | Top edge (around PD4) |
| (1,9) | XL OUT+ (5 V) + relay 5V | Top edge |
| (3,8) | Relay IN4 | Top edge |
| (4,8), (5,8), (6,8) | Cover LEFT, DOWN, OK | Top edge |
| (11,1) | XL7015 IN+ (bus) | Bottom edge |
| (11,2) | XL7015 IN− (C) | Bottom edge, over the empty (11,0) |
| (12,9) | COM 18 AWG (drill 1.3) | Top edge |
| Terminal (23,0)/(21,0) | Field R / field C, 18 AWG solid | Bottom edge (right corner) |

The relay cable splits: IN1–3 leave by the bottom edge; IN4, GND and 5 V by the top edge. The cover cable splits the same way. This was true in round 4 too.

### 5.3 Underside links

**Logic:** round 4's set; everything is a 1-pitch bridge or the part's own lead.

| Net | Holes | Material |
|---|---|---|
| UP | (0,0)–(0,1) | Lead offcut |
| SDA | (4,0)–(4,1) | Lead offcut |
| SCL | (5,0)–(5,1) | Lead offcut |
| IN1 | (1,0)–(1,1)–(1,2) | PD1's own signal lead, flat |
| IN2 | (2,0)–(2,1)–(2,2) | PD2's own signal lead |
| IN3 | (3,0)–(3,1)–(3,2) | PD3's own signal lead |
| GND | (1,6)–(1,7) | Offcut |
| GND | (3,6)–(3,5)–(2,5)–(1,5)–(1,6) | Offcut |
| GND | (1,7)–(1,8)–(2,8)–(2,9) | Offcut |
| 3V3 | (2,7)–(2,6) | Offcut |
| D6 | (6,1)–(6,2)–(5,2) | Offcut |
| RIGHT | (5,5)–(5,6) | Offcut |
| 5V | (0,7)–(0,8) | Offcut |
| 5VA | (0,9)–(1,9) | Offcut |
| IN4 | (3,7)–(3,8)–(3,9) | Offcut |
| LEFT, DOWN, OK | (4,7)–(4,8), (5,7)–(5,8), (6,7)–(6,8) | Offcut |

**Power:** all bare and flat, 0 insulated, 0 crossings.

| Net | Holes | Material |
|---|---|---|
| R | (23,0)–(23,1)–…–(23,9) | Bare 18 AWG strap down the right edge, lapped on the terminal R tail and both fuse-input tails. Column 22 stays empty. |
| F1o | (14,4)–(13,4)–(12,4) | The 1N4007 anode lead, flat |
| COM | (12,9)–(13,9)–(14,9) | The COM wire's own stripped end, flat to the F2-out tail |
| Bus | (11,1)–(12,1)–(13,1) | 1N4007 cathode lead to the TVS cathode and the XL IN+ landing |
| Bus | (13,1)–(13,0)–(14,0)–(15,0)–(16,0)–(17,0) | Bare 18 AWG to the cap + |
| C | (21,0)–(20,0)–(19,0) | Bare 18 AWG, terminal C to the cap − |
| C | (19,0)–(19,1)–(19,2)–(18,2)–(17,2)–(16,2)–(15,2)–(15,1) | Bare 18 AWG to the TVS anode; row 2 runs under the F1 holder edge |
| C | (15,2)–(14,2)–(13,2)–(12,2)–(11,2) | Offcut to the XL IN− landing |

The underside stack is one flat 18 AWG at most, about 1.2 mm; nothing crosses.

### 5.4 Keep-empty holes (never solder or bridge)

- **Antenna gap:** every hole in columns 7–10.
- **Column 22:** all ten holes (beside the R strap).
- **Power island:** (10,1), (11,0), (11,4), (11,9), (12,0), (12,3), (12,5), (12,8), (13,3), (13,5), (13,8), (14,1),
  (14,3), (14,5), (14,8), (15,4), (15,9), (16,1), (17,1), (18,0).
- **Logic:** (0,2), (0,6), (4,2), (4,5), (4,6), (4,9), (6,0), (6,6).

### 5.5 Adjacency hazards and how bring-up catches them

Each pair below would take one solder bridge.

| Pair | Effect | Caught by |
|---|---|---|
| IN1–IN2, IN2–IN3: (1,0)/(2,0)/(3,0) and (1,2)/(2,2)/(3,2), plus inherent header D1–D2–D3 | One relay drives another | Step 0: IN to IN > 1 MΩ |
| IN3–SDA: (3,0)/(4,0), plus inherent D3–D4 | O energised at boot | Step 0: IN3 to SDA > 1 MΩ. In service, I2C dies, so the sensor fault turns all outputs off within 2 min. |
| IN4–3V3: inherent D10–3V3 | W energised | Step 0: IN4 to 3V3 > 1 MΩ |
| Bus–C: (15,0)/(15,1), (11,1)/(11,2), (12,1)/(12,2), (13,1)/(13,2) | Same as a shorted TVS: F1 blows (safe) | Step 0: bus to C charges, not 0 Ω |
| GND–3V3 (2,5)/(2,6), GND–5V (0,8)/(1,8) | Rail short | Step 0: 3V3 and 5V to GND not shorted |
| R, F1o, COM | Audited: no pad of another net touches them, so no single bridge can short R ahead of a fuse or put AC on logic or the bus | Step 0: R open to C, bus and every logic net |

The 1N4007 leads are 3 pitches apart, so no single bridge can short the rectifier. The TVS and cap leads are 2 pitches apart with an empty hole between.

### 5.6 Build order

1. **Fuse holders.** Solder the header pins to the clip legs with each holder sitting in (14,4)/(23,4) and (14,9)/(23,9) as the jig. Tug-test and check ≤ 0.05 Ω. Measure the width (≤ 10.1; the gap is 2.7 mm) and the height. Take them out again.
2. **Trough, with the XIAO and headers off.**
   - PD1, PD2, PD3 lying, signal leads bent flat through (c,1) to (c,0).
   - The 1 k.
   - The GND links (3,6)…(1,6) and (1,6)–(1,7), 3V3 (2,7)–(2,6), D6 (6,2)–(5,2), RIGHT (5,5)–(5,6).
3. **Female headers on rows 1/7.** Use a spare XIAO as the jig, with the black spacer removed. The header joints capture the PD leads and the bridges. Flush-cut to ≤ 1.2.
4. **Top logic.** The 1N5819 and PD4 standing, the row-0 bridges and the rows 7–9 bridges.
5. **Power island, low to tall:**
   1. Drill (12,9) to 1.3 mm, and (21,0)/(23,0) if the terminal pins don't fit.
   2. Fit the terminal.
   3. Fit the fuse holders, then epoxy their bases.
   4. Fit the R strap on column 23.
   5. Fit the 1N4007, with its anode lead to (14,4).
   6. Fit the TVS standing.
   7. Fit the bus strap on row 0 and the C straps.
   8. Fit the cap **last** (+ at (17,0); check twice), then glue the cap, TVS and holder edge together.
6. **Bring-up.** Step 0 (ohm test), then step 1 (power stage alone, XL7015 set to 5.00 V).
7. **Cables, with the XIAO unplugged.** Land row 0 and rows 8–9, then the two trough wires from the left end, the XL IN pair, COM, and the field R/C. Fit the XIAO last.

### 5.7 Re-pin?

Not needed: every remaining relay-on pair is caught by step 0, as in round 4.

- **Option A (round 4's, a safety gain):** IN4→D0, UP→D10, SDA→D9, SCL→D8, LEFT→D4, DOWN→D5. It removes IN3–SDA and IN4–3V3. It costs `board.h`, `check_safety_sources.py` and the relay bench re-checks.
- **Option B (new, cheap, not a safety change):** SCL→D6, RIGHT→D5.
  - D6 is U0TXD. The ROM/bootloader TX burst only clocks SCL while SDA idles high, so there is no START and nothing happens on the bus.
  - The console is already USB-Serial-JTAG, since D6 is a key input today.
  - The 1 k and the trough RIGHT landing disappear; SCL lands at (6,0) and RIGHT at (5,0).
  - No relay pin moves, so the cost is `board.h` plus an I2C and keys bench check.

## 6. S2 (all parts ≤ 17 mm)

S2 is round 4 plus S1's trough tidy-up, with these changes:

| Part | Change |
|---|---|
| F1 | in (20,0), out (11,0) |
| F2 | in (20,5), out (11,5) |
| 1N4007 | Standing: body over A (12,8), cathode hairpin to (14,8) |
| TVS | K (14,9), A (19,9) |
| Terminal | R (23,9), C (21,9), entries +y (top edge) |
| Cap | Lying, − (21,2), + (21,4), body +x; 10.3 mm over the right edge, glue it |

**Links:**

| Net | Holes | Material |
|---|---|---|
| R | (23,9)→(23,0)→(21,0) | Insulated 18 AWG |
| R | (21,0)–(20,0)–(19,0) | Bare |
| R | Column 19, (19,0)…(19,5)–(20,5) | Bare 18 AWG |
| C | (21,9)–(21,8)–(21,7)–(22,7)–(22,6)…(22,2)–(21,2) | Bare 18 AWG |
| C | (19,9)–(20,9)–(21,9) | Bare |
| Bus | (14,9)–(14,8)–(15,8)–(16,8) | Bare |
| Bus | (16,8)→(21,6) | Insulated |
| Bus | Cap + lead (21,4)→(21,6) | Sleeved |
| F1o | 1N4007 anode lead (12,8)→(12,0), then (12,0)–(11,0) | Sleeved, then bare |
| COM | (11,9)→(11,5) | Sleeved, COM wire end |

**Landings:** XL IN+ (21,6), XL IN− (22,6), COM (11,9). The checker passes it with 0 crossings and a clean R audit. Full spec: `symp/spec_S2.json`, also embedded in `sympathizer_layout.json`.

## 7. Caveats (not verified)

- **XIAO body position.** It is assumed pin-centred; the antenna zone is assumed to be the last 5 mm of the PCB.
- **Fuse-holder base width.** It is taken as 10.0 mm. S3 relies on these plastic-to-plastic gaps: 0.06 mm cap to terminal, 0.16 mm cap to F1, and the TVS touching both F1 and the cap. The F1–F2 gap is 2.7 mm and the terminal–F1 gap 1.16 mm. Measure the holder base and the cap diameter before committing. If the cap and F1 interfere, the grid has no other position for them; the fallback is S2.
- **Terminal pins.** They are assumed centred in an 8 mm body depth.
- **The standing cap's 1.4 mm base overhang** and the 22.5 mm height are fine for the board but must be carried into the case work.
- **The score weights are my own.** The conclusions (S3 > S4 > S2 > r4) hold on the raw metrics in §4 without the score.
