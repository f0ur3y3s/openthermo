# Critic: openthermo controller board, round 5 (board only)

## What I read and what I built

**Inputs.** I read BRIEF.md, HARDWARE.md, r4_layout.json/.png, r4_synthesis.md and CLAUDE.md. I did not read the
sympathizer's files; this review is independent.

**Tools.** Everything is in `review5/critic_work/`:

| File | What it does |
|---|---|
| `geo.py` | Pose generators on the 24 × 10 grid with real envelopes (see the note below the table). |
| `layout.py` | The checker and scorer (detailed below the table). |
| `search.py` | Stage 1 and stage 2 of the search. |
| `refine.py`, `refine2.py` | Simulated-annealing refinement. |
| `hand.py`, `hand_variants.py` | Hand-seeded layouts, scored with the same checker. |
| `r4eval.py` | Scores the round-4 layout with the same scorer. |
| `report.py`, `final.py` | Exports the JSON, the PNGs and the hole maps. |

**Envelopes modelled in `geo.py`:**
- the XIAO in all 4 orientations at every position, with header strips, PCB, antenna zone and USB plug corridor;
- the fuse holders along x and along y, either end as the input;
- the 5.08 terminal in 4 orientations, with its entry face required at a board edge;
- the 470 µF standing, or lying in 4 directions with a 2 mm lead bend;
- the TVS lying (span 5–7) or standing (span 2);
- the diodes and resistors lying (span 3–4) or standing (span 1).

**What `layout.py` checks and scores:**
- overlaps of real envelopes, including a lead coming up under another body and tall parts under the XIAO or in the USB corridor;
- top-side landings assigned by min-cost matching;
- a BFS underside router that only takes orthogonal steps through free holes, so a routed board has **0 crossings by construction**; anything it cannot route becomes a penalised "insulated jumper";
- every orthogonal **and diagonal** pad pair classified by hazard;
- antenna, 24 VAC/bus-to-logic distances, landing egress, standing parts, overhang and height.

**Score** (lower is better):

| Term | Weight |
|---|---|
| Hazard severity | 1 |
| Power link length | 1.5 per pitch |
| Logic link length | 1 per pitch |
| Number of links | 0.5 each |
| Insulated jumpers | 15 each, plus 2 per pitch |
| Antenna | 30 + 8/mm if 24 VAC is within 10 mm; 15 + 5/mm if a metal part is within 5 mm; 12 + if the antenna end is not at an edge |
| 24 VAC–logic distance | 3/mm under 12 mm |
| Bus–logic distance | 2/mm under 10 mm |
| Landings | 0.25 per mm of egress; 3 if under the XIAO; 4 per extra edge a cable uses |
| Standing parts | 3–5 each |
| Overhang | 3/mm |
| Height | 4/mm over 17 |

**Hazard severities** (orthogonal neighbours; diagonal neighbours count ×0.3):

| Pair | Severity |
|---|---|
| Pre-fuse R next to anything else | 100 |
| 24 VAC next to logic | 100 |
| Bus next to logic | 80 |
| Fused 24 VAC next to the bus | 60 |
| IN next to a net that is high at boot (3V3, 5V, SDA/SCL, D6/RIGHT) | 40 |
| IN–IN | 10 |
| Supply–GND | 5 |
| IN–key | 3 |
| IN–GND | 0 |

## 1. The round-4 layout, judged as a board

Scored with the same tool, r4 totals **789.6**:

| Term | Value | Notes |
|---|---|---|
| Hazard | 464.5 | |
| Power links | 54 pitches (≈137 mm) | |
| Logic links | 28 pitches | |
| Underside links | 31 | |
| 24 VAC to the antenna zone | 5.3 mm | |
| Antenna penalty | 102.6 | |
| Landings | 55.8 | |
| Parts | 37.6 | mostly the cap overhang |

### Unsafe

1. **Pre-fuse R is diagonal to the bus.** F2's input (19,5) is R. It sits diagonally, 3.59 mm centre to centre (about 1.6–1.8 mm copper gap), from:
   - the cap's + lead at (20,4);
   - the XL IN+ landing at (20,6), which carries a stranded 26 AWG wire.

   The r4 netcheck only looked at orthogonal neighbours, so the claim "no pre-fuse R pad next to anything" is false diagonally. One bridge or one stray strand puts unfused, unrectified R across a polarised 470 µF: on the negative half-cycles it vents, limited only by the transformer. This is the worst pair on the r4 board.
2. **Three orthogonal bus–GND pad pairs** at (20,4)–(21,4), (20,6)–(21,6) and (20,7)–(21,7). The XL IN+ and IN− landings are orthogonal neighbours, each with a stranded wire. A bridge blows F1, and the step-0 ohm test catches it, but it is an avoidable trap.
3. **24 VAC near the antenna.**
   - The fuse clips and the COM riser sit 7.3 mm from the antenna end, and the F2 plastic 5.3 mm. That is below the brief's own 10 mm guidance.
   - The antenna end is not at a board edge (rule 4).
   - Both follow from the case forcing the USB to the left wall.
4. **Relay-on pad pairs: eight.** In each, one bridge puts a net that is pulled high at boot onto a relay input:
   - the 2 inherent header pairs, D3–D4 (IN3–SDA) and D10–3V3 (IN4–3V3);
   - an orthogonal replica of IN3–SDA in the landing row, (3,0)–(4,0);
   - 5 diagonal replicas: (3,1)–(4,0), (3,0)–(4,1), (3,2)–(4,1), (2,7)–(3,8) and (2,6)–(3,7).
5. **The 1 k sits under the antenna end** at (6,2)→(6,6), at x = 15.2 mm, the column of the last pin pair.

### Only there because of the case (now pure cost)

- **The cap overhangs the right edge by 7.8 mm and is glued.**
  - The leads are bent 1.2 mm from the bung. That is tighter than the usual ≥2 mm keep-away from the seal.
  - Without a case it is an unsupported, lever-loaded can on the board edge.
- **F1 overhangs the bottom edge by 1.43 mm.**
- **The terminal entries face the top edge, with R at the top-right corner while both fuse inputs are at the bottom right.** R is the longest net on the board:
  - 11 pitches of insulated 18 AWG;
  - plus 2 + 6 pitches of bare strap;
  - 19 pitches in all, about 48 mm of unfused copper on the underside.
- **USB at the left short edge**, which turns the antenna into the board and burns columns 7–9 (30 holes, 12.5 % of the board) as keepout.
- **Lane L and trough routing.**
  - The 3V3 landing (2,6) and RIGHT (5,6) are threaded under the XIAO.
  - XL IN sits mid-board at (20,6)/(21,6), 11–13 mm from any edge.

### Awkward to build

- **PD1 and PD2 lie diagonally, with the bodies touching (0.06 mm).**
- **PD1–PD3 signal leads cross the header-pin pads.** Each lead is "bent underneath through (c,1)", which is the female-header pin hole. The resistors go in at build step 2, before the headers at step 3, so the lead lies across the pad of a hole the header pin must still pass through. It has to be dressed round the hole ring or the header won't seat.
- **Standing safety part.** PD4 is a pull-down standing at (3,9) with a bare hairpin on the board edge. The 1N5819 also stands.
- **Long lead and wire runs underneath:**
  - the 1N4007 anode lead is sleeved for 8 pitches down column 11, under both fuse holders;
  - the COM stripped end is sleeved 4 pitches and lap-soldered to a header-pin tail, so a fuse-holder change means desoldering COM;
  - a diagonal bus lead runs (13,9)→(14,8);
  - an insulated 18 AWG link runs diagonally (15,8)→(20,7).
- **Ground relies on the XL7015 harness.** Logic GND joins C only through the XL7015 harness (IN−/OUT−). That is acceptable as a star, but the step-0 test never checks that C equals XIAO GND.

## 2. From scratch: what the search found

**Search space.**
- XIAO: o ∈ {USB −x, USB −y} at every position, 144 poses. USB +x and +y are the same boards rotated 180°.
- USB face allowed up to 45 mm inside the board, provided the plug corridor (12 × 25 mm at z ≥ 7.5) is clear.

**Stage 1.** For each XIAO pose, the best 25 power islands (F1, F2, terminal, cap) by heuristic, out of 348 × 348 × 208 × 1456 raw combinations pruned by overlap and antenna limits. That gave 1300 islands.

**Stage 2.** A beam placement of the 1N4007, TVS, 1N5819, 1 k and PD1–4, followed by landings, routing and scoring, on the 440 best combinations.

**Stage 3.** Simulated annealing (2 × 6000 moves) on the top families. Its results:

| XIAO family (o, c, r) | Default pins | Re-pin |
|---|---|---|
| **V-left: USB bottom edge, antenna top edge (2,1,3)** | **443.9** | **311.6** |
| V-right, antenna top (2,16,3) | 475.8 | 312.5 |
| V-left, c=2 (2,2,3) | 486.6 | 327.0 |
| V-left, antenna not at the edge (2,1,1) | 468.7 (stage 3a) | — |
| **H, the r4 family: USB left edge, antenna inward (0,1,1)** | 506.6 | 360.1 |
| H (0,0,0) | 565.5 | — |
| USB pointing inward (o=0, c=17) | no fit; the plug corridor kills the fuse placements | — |

**Findings that drove the final design:**

1. **Vertical XIAO is the only pose that puts the antenna at a board edge with the USB still usable.**
   - The antenna end sits 0.7 mm inside the top edge.
   - The USB face is 7 mm inside the bottom edge. The plug overhangs only low parts.

   It wins in both pin maps. The cost: one pin row faces the power side, so logic needs a 2-column lane (columns 8–9).
2. **The power island that fits everything without overhanging the cap:**
   - The fuses lie along x, stacked and touching (gap 0.16 mm), on rows 5 and 9. F2 overhangs the top edge by 1.43 mm.
   - The 470 µF lies in the 10 mm strip underneath.
   - The terminal sits at the right short edge with entries facing out.

   Fuses along y each burn a full-height 4-column band and bury both pins under the body. Mixed orientations collide.
3. **The fuse output ends must face inward (column 14).** A holder's end pins sit under its own body, so the COM wire can only land in the hole just past the body end (column 13). Facing the inputs (R) to the right short edge gives R a 7-pitch strap down the edge column, away from everything.
4. **The automatic logic placement is noisy.** With the beam and simulated annealing it still leaves 1–3 insulated jumpers. My hand-seeded logic islands, scored with the same checker, beat it. They are what I propose.

**Final comparison** (same scorer; hand layouts, routed by the BFS router, 0 jumpers, 0 crossings):

| Layout | Total | Hazard | Relay-on pairs (orth + diag) | Power / logic link pitches | 24 VAC→antenna | 24 VAC→logic | Antenna at edge |
|---|---|---|---|---|---|---|---|
| r4 (case-constrained) | 789.6 | 464.5 | 8, plus 2 R–bus diagonals | 54 / 28 | 5.3 mm | 10.5 mm | no |
| **#1 V-left, 1 k in column 9 (default pins)** | **434.8** | 250.1 | 6 (2 inherent + 4 diagonal) | 26 / 24 | 12.7 mm | 7.6 mm (F1o→RIGHT, 2 empty holes between) | **yes** |
| #2 V-left, 1 k along row 9 | 453.5 | 269.4 | 6 | 26 / 25 | 12.7 | 5.1 (COM→RIGHT) | yes |
| #3 H (r4 XIAO pose, same power island) | 466.7 | 267.5 | 6 | 26 / 23 | 12.4 | **15.2** | no (keepout columns 7–9) |
| #1-R V-left with the re-pin | **325.2** | 153.0 | **0** | 26 / 23 | 12.7 | 7.6 | yes |
| #3-R H with the re-pin | 383.6 | 195.9 | 0 | 26 / 26 | 12.4 | 15.2 | no |

**Choosing between #1 and #3.** The scorer prefers #1 because the antenna sits at an edge. #3 keeps fused 24 VAC 15 mm from logic, against 7.6 mm in #1, but points the antenna into the board and leaves the 1 k under the antenna end. Neither has any 24 VAC or bus pad adjacent (orthogonally or diagonally) to logic. I recommend **#1**; #3 is the fallback if the user rates radio below that 8 mm.

The files:

| Layout | JSON | Picture |
|---|---|---|
| #1 | `critic_layout.json` | `critic_layout.png` |
| #1-R | `critic_layout_repin.json` | `critic_alt_repin.png` |
| #3 | `critic_layout_H.json` | `critic_alt_H.png` |

## 3. Best layout (#1): V-left, default pin map

**Frame.** Hole (c, r) is at (2.54c, 2.54r) mm. The board spans x −5.8..64.22 and y −3.57..26.43, with top z = 0. The r4 case frame is x − 58.39, y − 46.41.

```
       0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
r9    OK OK  x  x  x  x  x d6 d6 d6  .  .  . CM CM  .  .  .  .  .  .  .  . R      <- top edge: antenna (x), COM
r8    Dn Dn  .  .  .  .  . SC SC  .  .  .  .  .  .  .  .  .  .  .  .  .  . R      F2 body rows 7.0-11.0 (cols 13.2-23.8)
r7    Lf Lf  .  .  .  .  . SA SA  .  .  .  .  .  .  .  .  .  .  .  .  .  . R
r6    i4 i4  . g   .  . i3 i3  . Rt  .  . f1 f1  .  .  .  .  .  .  .  .  . R      F1 body rows 3.0-7.0
r5     . 3V 3V g   .  . i2 i2  .  .  .  .  . f1 f1  .  .  .  .  .  .  .  . R
r4    g  g  g  g   .  . i1 i1  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . R
r3    g  5V  .  .  .  .  . Up  .  .  .  . B+  .  .  .  .  .  .  .  .  .  . R
r2    g  5V  .  .  . 5A  . Up  .  .  .  . B+ B+ B+ B+ B+ B+ B+ B+ B+  .  . R      cap body rows -1..3, cols 9.3-17.2
r1    g   .  .  .  . 5A  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . G   .  .
r0    g   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . G  G  G  G  G  G      <- bottom edge: USB plug, relay cable
```

Abbreviations: g/G = logic GND / C; B+ = bus; f1 = F1 out; CM = COM; R = pre-fuse R.

### Parts

Envelopes are grid-frame mm, x0..x1 × y0..y1 × height.

| Part | Lead holes | Orientation | Envelope |
|---|---|---|---|
| XIAO on 2 × 7 female headers | **row A, column 7:** D0..D6 at (7,3)..(7,9). **Row B, column 1:** 5V GND 3V3 D10 D9 D8 D7 at (1,3)..(1,9) | USB to the bottom edge (face 7.0 mm inside); antenna end 0.7 mm inside the top edge | PCB 1.3..19.1 × 4.7..25.7, z 8.5–13; headers 8.5 tall |
| PD1 10k (IN1) | signal (6,4), GND (2,4) | lying along x, under the XIAO, span 4 | 7.0..13.3 × 9.0..11.4 × 2.4 |
| PD2 10k (IN2) | signal (6,5), GND (3,5) | lying along x, span 3 | 8.3..14.6 × 11.5..13.9 × 2.4 |
| PD3 10k (IN3) | signal (6,6), GND (3,6) | lying along x, span 3 | 8.3..14.6 × 14.0..16.4 × 2.4 |
| PD4 10k (IN4) | signal (0,6), GND (0,2) | lying along y in column 0, span 4 | −1.2..1.2 × 7.0..13.3 × 2.4 (touches the header plastic) |
| 1N5819 | K (1,2), A (5,2) | lying along x on row 2, under the PCB edge | 5.0..10.2 × 3.7..6.4 × 2.7 |
| 1 k (D6→RIGHT) | D6 (9,9), RIGHT (9,6) | lying along y in column 9, span 3 | 21.7..24.1 × 15.9..22.2 × 2.4 |
| F1 T1A | in (23,5), out (14,5) | along x | 33.6..60.4 × 7.7..17.7 × 17 |
| F2 T1.6A | in (23,9), out (14,9) | along x | 33.6..60.4 × 17.9..27.9 × 17 (**overhangs the top edge 1.43 mm**) |
| Terminal 5.08 | **R (23,2), C (23,0)** | entries face the right short edge (1.8 mm inside it) | 54.4..62.4 × −2.6..7.6 × 12 (assumes pins centred in its 8 mm depth) |
| 470 µF 63 V | **+ (18,2), − (18,0)** | lying, body toward −x, leads bent 2.0 mm from the bung | 23.7..43.7 × −2.5..7.5 × 10.2. Fully on the board; glue optional |
| 1.5KE51A TVS | A (20,0) under the body, K (20,2) hairpin | standing; **sleeve the hairpin**; drill both holes 1.3 | 48.1..53.4 × −2.6..2.6 × ~14 |
| 1N4007 | A (12,6), K (12,3) | lying along y in the pocket left of F1 | 29.1..31.8 × 8.8..14.0 × 2.7 |

**Off the board** (same as r4): the 100 nF parts, the I2C pull-ups, the NPN drivers.

### Off-board connections

All land on the top side. Two conductors in one hole means drilling it to 1.3 mm. Holes marked † share a hole with a part lead.

| Hole | Wire | Edge | Distance to edge |
|---|---|---|---|
| (6,4)†, (6,5)†, (6,6)† | relay IN1, IN2, IN3 | bottom. The wires enter the trough from the USB end, run up the 3 mm channel between the PD ends (x ≤ 14.6) and the D-row header (x ≥ 16.6), and drop into the PD signal holes | 11–14 mm, under the XIAO |
| (0,6)† | relay IN4 | left | 5.8 |
| (5,1) | relay 5V + XL OUT+ (twisted) | bottom | 6.1 |
| (0,1) | relay GND + XL OUT− (twisted) | left / bottom corner | 5.8 |
| (0,0) | cover GND + SHT GND | bottom-left corner | 3.6 |
| (2,5) | cover 3V3 + SHT 3V3. A trough landing, from the USB end | bottom | about 14 |
| (7,2) | cover UP (under the PCB edge) | bottom | 8.7 |
| (0,7), (0,8), (0,9) | cover LEFT, DOWN, OK | left | 5.8 |
| (8,7), (8,8) | cover SDA, SCL (+ SHT, twisted) | top. Runs 1.2 mm beside the antenna zone; dress the wires away from the XIAO | 8.6 / 6.1 |
| (9,6)† | cover RIGHT | top (over the 1 k) | 11.2 |
| (19,2) | XL IN+ (bus). Wire from above, between the cap end and F1 | bottom / top | 8.7 |
| (21,1) | XL IN− (C) | bottom | 6.1 |
| (13,9) | COM, 18 AWG. **Drill 1.3** | top | 3.6 |
| Terminal | field R and C, 18 AWG solid | right | — |

### Underside links

There are 30, all bare, **0 insulated, 0 crossings**. The power links total 26 pitches, against 54 in r4. The logic links total 24 pitches, all 1–3 pitch.

**Power** (bare 18 AWG, or the part's own lead where stated):

| Net | Holes | How |
|---|---|---|
| R | (23,2)–(23,5)–(23,9) | 7 pitches down the right-edge column; lap onto the terminal R tail and both fuse-input tails. Every pad in column 23 rows 3–9 sits under a holder body. |
| COM | (13,9)–(14,9) | The COM wire's own stripped end, 1 pitch, lapped to the F2-out tail |
| F1o | (12,6)–(13,6)–(13,5)–(14,5) | The 1N4007 anode lead bent on the underside, 3 pitches |
| Bus | (12,3)–(12,2)–…–(18,2) | 7 pitches along row 2 under the cap. The 1N4007 cathode lead (about 25 mm) can be the link. |
| Bus | (18,2)–(19,2)–(20,2) | Cap + to the XL IN+ landing to the TVS cathode |
| C/GND | (23,0)–(22,0)–(21,0)–(20,0)–(19,0)–(18,0) | Terminal C to the TVS anode to cap − |
| C/GND | (21,0)–(21,1) | XL IN− |

**Logic** (1-pitch solder bridges or lead offcuts):

| Net | Holes |
|---|---|
| IN1, IN2, IN3 | (7,4)–(6,4), (7,5)–(6,5), (7,6)–(6,6). Use the PD signal lead, bent to the pin tail. |
| IN4 | (1,6)–(0,6) |
| GND | (1,4)–(2,4)–(3,4)–(3,5)–(3,6) |
| GND | (1,4)–(0,4)–(0,3)–(0,2)–(0,1)–(0,0) |
| 5V | (1,3)–(1,2) |
| 5A | (5,2)–(5,1) |
| 3V3 | (1,5)–(2,5) |
| SDA | (7,7)–(8,7) |
| SCL | (7,8)–(8,8) |
| D6 | (7,9)–(8,9)–(9,9), 2 pitches |
| UP | (7,3)–(7,2) |
| LEFT, DOWN, OK | (1,7)–(0,7), (1,8)–(0,8), (1,9)–(0,9) |

### Keep empty

Never solder or bridge these.

- **Logic:** (0,5), (2,6), (2,9)…(6,9) (the antenna end), (6,7), (8,6).
- **Pocket:** (11,2), (11,3), (11,6), (12,1), (12,4), (12,5), (12,7), (12,9), (13,1), (13,3), (13,4), (13,7), (13,8).
- **Strip:** (14..19,1), (14..20,3), (14,4), (14,6), (14,8), (15,5), (15,9), (20,1), (21,2).
- **R guard:** all of column 22 rows 2–9, and (23,1), which sits between R and C.

### Adjacency hazards and how bring-up catches them

**Pre-fuse R.** No neighbour, orthogonal or diagonal, except empty holes. The nearest other-net pads are:
- C (23,0), at the terminal's own 5.08 mm pitch;
- the bus (20,2), at 7.6 mm.

**F1o and COM.**
- They touch no logic pad, bus pad or each other.
- The closest logic is RIGHT (9,6), 7.6 mm from F1o (12,6) with two keep-empty holes between.
- The antenna zone is 12.7 mm away; metal parts are 12.1 mm away.

**Bus vs C.** One diagonal pair: TVS K (20,2) to XL IN− (21,1). A bridge blows F1, and "bus to C charges, not 0 Ω" catches it before power.

**Relay-on pairs.** These are the pairs where one bridge can hold a relay on at boot:

| Pair | Type |
|---|---|
| IN3–SDA (7,6)–(7,7) | header, inherent |
| IN4–3V3 (1,5)–(1,6) | header, inherent |
| IN3 pin–SDA landing (7,6)–(8,7) | diagonal |
| PD3/IN3 landing–SDA pin (6,6)–(7,7) | diagonal |
| IN4 pin–3V3 landing (1,6)–(2,5) | diagonal |
| IN4 landing–3V3 pin (0,6)–(1,5) | diagonal |

There is **no orthogonal landing replica** (r4 had (3,0)–(4,0)). Two of the header pairs are worse than they look:
- **IN4–3V3** energises W at power-on. Then the bootloader drives D10 low into 3V3, so the 3V3 rail collapses into a reset loop and W chatters.
- **IN3–SDA** holds O on at boot. Then I2C dies, and the sensor fault drops everything within 2 min.

All are caught only by the **mandatory step-0 ohm test**: each IN to 3V3, SDA, SCL, D6, RIGHT and 5V must read > 1 MΩ.

**Lower-severity pairs:**

| Pairs | Effect |
|---|---|
| IN1–IN2, IN2–IN3 (header, and the trough holes (6,4)–(6,5)–(6,6)) | Cross-drive relays, not at boot. Ohm test IN-to-IN. |
| IN1–UP, IN4–LEFT (header, and (0,6)–(0,7)) | Key lines at about 0.6 V. Add "IN to every key line > 1 MΩ" to step 0. |
| 5V/3V3–GND at (1,3)–(1,4), (1,4)–(1,5), (2,4)–(2,5), (2,5)–(3,5), (0,2)–(1,2), (0,3)–(1,3) | Already in step 0: 3V3 and 5V not shorted to GND. |

**Add to step 0:**
- C to XIAO GND = 0 Ω with the XL harness connected. The two grounds join only through the XL7015.
- R to every column-22 pad: open.

### Build order

1. **Drill to 1.3 mm:**
   - (13,9) for COM;
   - (20,0) and (20,2) for the TVS;
   - the shared lead-plus-wire holes (6,4), (6,5), (6,6), (0,6), (9,6);
   - (23,0) and (23,2) if the terminal pins bind.
2. **Fuse holders.** Solder the header pins to the clips with the holder seated in the board as a jig. Tug-test, check ≤ 0.05 Ω, then lift them out.
3. **Power straps while the board is bare:**
   - the R strap in column 23;
   - the C strap along row 0.
4. **Terminal, then the fuse holders.** Solder the holders and epoxy their bases.
5. **1N4007.** Bend the anode lead to the F1-out tail; run the cathode lead (or 18 AWG) along row 2.
6. **TVS** (anode down at (20,0), sleeved cathode hairpin into (20,2)).
7. **470 µF last on the power side.** Bend the leads ≥ 2 mm from the bung; glue optional.
8. **Bring-up step 1** on the power stage alone: set the XL7015 to 5.00 V. If it fails now, nothing logic-side is at risk.
9. **Logic parts, all lying:** PD1–PD4, the 1N5819, the 1 k, and their 1-pitch bridges. The PD signal leads end on the pin pads; no lead crosses a header hole.
10. **Female headers.** Use a spare XIAO as the jig.
11. **Landings, with the XIAO out:**
    1. the trough wires first: IN1–IN3 and 3V3 from the USB end;
    2. then the left-edge, bottom-edge and top-edge wires;
    3. XL OUT at (5,1)/(0,1), XL IN at (19,2)/(21,1).
12. **Step-0 ohm test** with the extended list above, **then** plug in the XIAO.

### Weak points I'm flagging against my own layout

- **The 1.43 mm F2 overhang** on the top edge. This is the space cost of keeping the cap fully on the board.
- **4 trough landings** (IN1–IN3, 3V3). Their wires run under the XIAO from the USB end. They are short, but r4's trough complaint applies to them.
- **F1o is 7.6 mm from RIGHT.** r4's worst 24 VAC–logic distance was 10.5 mm.
- **Unchecked assumptions:**
  - the terminal pins are centred in its depth (if they sit offset, the face moves up to ±1.5 mm);
  - the TVS leads (about 1.0–1.2 mm) need the 1.3 mm holes;
  - SDA and SCL wires run 1.2 mm beside the antenna zone (keep them flat and leave toward the top edge away from the XIAO).

## 4. Firmware re-pin: does it help, and by how much

**Yes, materially.** The re-pin is IN4→D0, UP→D10, SDA→D9, SCL→D8, LEFT→D4, DOWN→D5. On the same V-left board (#1-R):

| Measure | #1 | #1-R |
|---|---|---|
| Hazard | 250 | 153 |
| Score | 435 | 325 (−25 %) |
| Relay-on pad pairs | 6 | **0** |

- All four IN pins move to the D0–D3 USB end, so the pull-downs become four identical parallel resistors under the XIAO, (6,3..6)→(2,3..6).
- The B side becomes 5V, GND, 3V3, UP, SDA, SCL and OK, all landing straight out at column 0.
- No IN sits next to a pulled-high net anywhere: header, landings or diagonals. The step-0 ohm test becomes a check rather than the only guard against W or O at boot.

A smaller 4-pin swap gives the same zero: IN4↔UP (D0↔D10) and SDA→D9, LEFT→D4.

**The cost:**
- `board.h`;
- the expectations in `tools/check_safety_sources.py`;
- re-running the reset, panic and bootloader-hook relay bench checks;
- scoping IN4 (now GPIO0, an LP/XTAL_32K pin) through power-on and reset to confirm it stays below 1 V.

The relay cable order at the board changes too.
