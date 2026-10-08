# Synthesis: openthermo controller board (round 4)

Inputs: BRIEF.md, HARDWARE.md, current_layout.png, `fusion_case_v3.py`, CLAUDE.md, critic.md (with critic_work/check.py) and
sympathizer.md (with symp/geo.py and symp/netmap.py).

My own checks are in `scratchpad/synth/`:
- `netcheck.py`: hole-to-net map, every orthogonal pad adjacency classified by hazard, support-pad holes, keepout holes,
  and underside link crossings in plan view.
- `geocheck.py`: 3D clearance of every part, wire envelope, cover pin and fence lip against each other and against the
  enclosure features.

The machine-readable spec is `synthesis_layout.json`.

## Verdict

Both reviews are half right.

- **The critic is right that the current plan does not build.** It has about 19 cable conductors soldered to header tails
  in a 3 mm gap, plus unspecified long jumpers.
- **The sympathizer is right that the floor plan is sound.** The logic / antenna gap / 24 VAC zoning, the fuse
  orientation, the overhanging cap and the cradle are all good.
- **The fix needs no firmware re-pin.** Move the XIAO up one grid row and land every cable wire on top-side holes, each
  joined to its XIAO pin by a 1-pitch underside bridge.

The final layout has:
- **zero cable conductors and zero crossings on the underside**;
- **no new pad adjacency that can turn a relay on, except one** (IN3–SDA at row 0, the same pair the header already has);
- **no pre-fuse R pad next to anything**.

The critic's re-pin is a real but modest safety gain. It is listed below as a separate, optional decision.

## 1. Rulings

### Critic

| # | Finding | Ruling | Basis (my check) |
|---|---|---|---|
| B1 | The underside can't hold ~35 conductors | **CONFIRMED** for the plan as drawn | 19 cable conductors land on 14 tails trimmed to 2 mm, which leaves 1.0 mm under each tail. A relay IN wire from the top-left to a row-0 tail must cross the cover wires going to row-6 tails and the 5V/GND/3V3 straps. Each crossing is 2.6 mm (two 1.3 mm wires). Also, the XL 5 V (≈48 mm) and C→XIAO GND (≈56 mm) links are required by the circuit but drawn nowhere. |
| M1 | 24 VAC copper over logic wires underneath | **CONFIRMED** (current) | The SHT40 cable runs flat under F1/F2 pins, the cap leads and the R links. Final: no logic conductor under the power island, and the SHT40 goes up the right gutter. |
| M2 | Pre-fuse R pad (19,1) next to cap lead (20,1) | **CONFIRMED** | Pitch 2.54, copper gap ≈0.7–1.0 mm. A bridge puts R onto the cap unfused (it vents), or makes an R–C short protected only by the transformer. Sympathizer W6 calls it unavoidable; it is not (§2). |
| M3 | Relay-input adjacency hazards | **CONFIRMED.** The fix is **OVERSTATED** as needing a re-pin | Current: the IN4 pull-down lead (0,1) sits between D0 and IN1. Inherent to the pin map: D10–3V3 and D3–SDA. Final: only those inherent header pairs remain, plus one added IN3–SDA pair at row 0, which is unavoidable without lap joints (§3). All are caught by a mandatory ohm test. |
| M4 | Wires on female-header tails are unreliable and unserviceable | **CONFIRMED** | Final: every wire goes through its own hole from the top. Hole-in-pad joints give strain relief, and the tails carry only a bridge. |
| M5 | Board not retained with the cover off; 1.07 x play on the left | **CONFIRMED** | Final: a 0.5 lip on every fence, plus two 0.92 bumps on the left rim. Sympathizer W7 ("hold it with a finger") is inadequate while R/C are screwed into a horizontal-entry terminal. |
| M6 | Fuse holders mechanically marginal | **CONFIRMED** (cheap fixes) | Solder the pins with the holder in the board as a jig (23.0 vs 22.86), epoxy the bases, and measure `FUSE_H`. |
| M7 | 18 AWG won't pass a 1.0 hole | **CONFIRMED** | Fix: drill (10,9) to 1.3 mm. The COM wire's own sleeved stripped end is the link to the F2 pin (sympathizer §3.5). The critic's "solder to the leg on top" doesn't work cleanly: the clip end faces −x, so the wire would run through the antenna gap. |
| M8 | USB-C reach marginal | **CONFIRMED** | Face at −62.5 (pin-centred) vs wall inner face −67: the overmold sits 4.5 mm into a 13 × 7.5 cut. Final cut: 15 × 9 (z 14.25–23.25), plus the spacer-removal note. |
| m1 | Cap overhang; bend 1.5 | **CONFIRMED, amended** | The critic missed the sensor grommet at x 14.6: a 1.5 bend leaves only 0.69 to it. Use a **1.2** bend: 0.99 to the grommet, 1.14 to the rib. |
| m2 | XL only 2 holes for 3 wires | **CONFIRMED** | Final: the XL harness is split, IN pair to the power island and OUT pair to the logic island. |
| m3 | XIAO body likely pin-centred | **PLAUSIBLE, unverified** | Seeed's footprint is blocked from this session. The spec uses pin-centred (body x −61.27..−40.27) and gives the CAD-offset numbers too. Measure from the USB face to the D0 pin. |
| m4 | Antenna 6–7 mm to fuse clips | **CONFIRMED** (acceptable) | 7.28 mm (pin-centred) or 6.01 mm (CAD). The U.FL + FPC fallback stands (both reviewers agree). |
| m5 | Drop the 3V3 100 nF | **CONFIRMED.** The 5V one is dropped too | The XIAO decouples its own rails. |
| m6 | Pull-ups as drawn are bad | **CONFIRMED** (both agree) | Off the board. |
| m7 | Keep pad-adjacent holes empty | **CONFIRMED and met** | The netcheck finds no used hole within reach of a pad. |
| m8 | Relay harness Ø3.7 in a 3.25 channel | **CONFIRMED** (pre-existing) | Dress it flat, 2 × 3. The round CAD model shows −0.63 against the module. |
| m9 | Cover bundle over the wall screw | **CONFIRMED, minor** | No 3D clash: the bundle is at z ≥ 15.7 and the head at z ≤ 7.5. It only blocks the driver. Drive the screws before dressing the bundle, or push it aside. |
| m10 | COM route | **CONFIRMED** | Final: the riser is at (10,9), 7.3 mm from the antenna end, and the run is at z 23, above the cover bundle. |
| m11 | Pull-downs touching | Accept | Final: PD1 and PD2 run diagonally and parallel, 0.06 apart. The bodies are insulated. |
| P1-R2 | Re-pin (IN4→D0, UP→D10, SDA→D9, SCL→D8, LEFT→D4, DOWN→D5) | **REJECTED as default**, offered as an option (§6) | The benefit is real but modest; see §6. |
| P1-R2 | 1k standing at (6,0) | **REJECTED** | The body edge is 0.06 mm from the XIAO PCB edge (y −45.15), and the hairpin reaches z ≈ 17 > 16.1 (PCB underside). |
| P1-R2 | Power-island links "6 insulated, all inside the island" | **OVERSTATED** | The bus (13,9)→(20,4) crosses the R link (19,5)→(19,1). The TVS-anode link (19,9)→(23,7) runs over the R pad (21,8). Final: a planar set with 0 crossings. |
| P1-R2 | Pin at (−63.2,−44.5) | **REJECTED** | It sits in the only wire lane past the USB end. Replaced by fence lips. |

### Sympathizer

| # | Finding | Ruling | Basis |
|---|---|---|---|
| 1.1–1.5 | Zoning, fuse orientation, cap overhang, cradle | **CONFIRMED** | Kept as is. |
| 1.6 | Header-tail wiring is "standard and sound" | **OVERSTATED** | It is sound for 2–3 wires, not 19 in a 3 mm gap (B1). |
| W1 | Pull-ups don't fit | **CONFIRMED** | Off the board. |
| W2 | Cross-board wiring unspecified | **CONFIRMED** | The XL OUT pair rides the cover bundle (both reviewers agree). |
| W3 | Tails in a 3 mm gap; optional backplate trough | **CONFIRMED.** The trough is **not needed** | Final logic underside ≤ 1.2 mm. |
| W4 | F1/F2 only 0.16 apart | **CONFIRMED as a risk** | **Adopted:** F1 to row 0. Gap 2.7, overhang 1.43 into the gutter, 0.84 to the (−37,−48.6) pin. |
| W5 | USB height depends on the spacer | **CONFIRMED** | Remove the black spacer so the XIAO PCB sits at 8.5. |
| W6 | R/cap adjacency unavoidable | **REJECTED** | Cap on rows 2/4, F1 on row 0, R strap on column 18: every R pad has only empty or same-net neighbours (netcheck: 0 BLOCKER). |
| §3.1 | Land IN wires at the pull-down | **Adopted in spirit** | Each pull-down's signal lead is bent along the underside as the bridge (c,2)→(c,1)→(c,0). Landing, pin and pull-down are one continuous lead. |
| §3.2 | SHT40 via the bottom gutter | **REJECTED** | Its GND/3V3 would still have to cross to the trough or top area. The right gutter puts every logic cable at one corner. |
| §4 | Map with "0 long underside wires" | **OVERSTATED** | 15 cable conductors still run under the XIAO, and the bus jumper (14,8)→(20,3) crosses the column-18 R strap underside. |
| §3.5 | Drill (10,9) for COM | **CONFIRMED, adopted** | — |
| §3.6 | 1k in column 6 | **Adopted** | (6,2)–(6,6) under the XIAO, 7 mm below its PCB. A slow key line; the D6/D7 header pins are already at that x. |

### Points the brief asked about specifically

- **Real underside count and stack height:**

  | Plan | Cable conductors underneath | Insulated jumpers | Worst stack (3.0 gap) |
  |---|---|---|---|
  | Current | 19 | ≈10 | 2.6–2.8 on 2.0 tails: does not fit |
  | Sympathizer map | 15 under the XIAO | 2 | plus 1 power-island crossing (≈3.0): marginal |
  | **Final** | **0** | power island only (two 18 AWG, two sleeved leads), **0 crossings** | logic ≤ 1.2 (tails flush-cut, 1-pitch bridges); power ≤ 2.2 (one 18 AWG, flat) |

- **XIAO geometry.** Pins 0.6″ DIP, 2.54 pitch, 7 per row. The D0–D6 row is on the −y side with the USB facing −x (component side up), which fixes the D-row at the bottom. The body is assumed pin-centred. The antenna is at the +x end per the brief. If the body is actually as the CAD has it (−60..−39), every antenna distance drops 1.27 mm and the USB face moves +1.27 (deeper). Both cases are in the JSON.
- **USB-C:** see M8 and W5.
- **Retention:** see M5.
- **COM:** see M7.

## 2. Final layout (default, no re-pin)

- **The board does not move.** It is still 30 × 70, x −64.18..5.82, y −49.98..−19.98, top z 7.6.
- **XIAO:** rows **1** (D0..D6) and **7** (5V GND 3V3 D10 D9 D8 D7), columns 0–6, `XIAO_CY = PB_Y0 + 4·P = −36.25`. Row 0
  (below) and rows 8–9 (above) become outside landing holes, and rows 2–6 are the trough under the XIAO.
- **Columns 7–9 stay empty on both sides** (antenna keepout).

Top view, row 9 at the top. Abbreviations: i1–i4 = IN1–IN4; SA/SC = SDA/SCL; 5A = the 1N5819 anode net (XL 5 V and relay
5 V); G = GND/C; R = 24 VAC R; f1 = F1 out; CM = COM; B+ = DC bus; Lf/Dn/Rt = keys. x = antenna keepout.

```
      0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
r9   5A 5A G  i4  .  .  .  x  x  x CM  .  . B+  .  .  .  .  . G  G  G   .  .
r8   5V G  G  i4 Lf Dn OK  x  x  x  . f1  .  . B+ B+  .  .  .  .  . G   . R
r7   5V G  3V i4 Lf Dn OK  x  x  x  .  .  .  .  .  .  .  .  .  . B+ G   .  .     <- XIAO row 7
r6   G  G  3V G   . Rt Rt  x  x  x  .  .  .  .  .  .  .  .  .  . B+ G   .  .
r5    . G  G  G   .  .  .  x  x  x CM  .  .  .  .  .  .  . R  R   . G   .  .
r4    .  .  .  .  .  .  .  x  x  x  .  .  .  .  .  .  .  . R   . B+ G   .  .
r3    .  .  .  .  .  .  .  x  x  x  .  .  .  .  .  .  .  . R   .  . G   .  .
r2    . i1 i2 i3  .  . d6  x  x  x  .  .  .  .  .  .  .  . R   . G  G   .  .
r1   up i1 i2 i3 SA SC d6  x  x  x  .  .  .  .  .  .  .  . R   .  .  .  .  .     <- XIAO row 1
r0   up i1 i2 i3 SA SC  .  x  x  x f1 f1  .  .  .  .  .  . R  R  R   .  . R
```

(23,0) is only the corner of the insulated R wire, not a joint.

### Parts

Envelopes are in the JSON.

| Part | Holes | Notes |
|---|---|---|
| PD1 10k (IN1) | (1,2) → (0,6) | Lying, diagonal (lead span 10.47) |
| PD2 10k (IN2) | (2,2) → (1,6) | Diagonal, parallel to PD1 |
| PD3 10k (IN3) | (3,2) → (3,6) | Lying along y |
| 1k (D6, RIGHT) | (6,2) → (6,6) | Lying along y |
| PD4 10k (IN4) | body standing over (3,9), hairpin to (2,9) | Top z ≈ 15.6 |
| 1N5819 | anode body standing over (0,9), cathode hairpin to (0,8) | The 0.6″ header strip leaves no room to lie it in row 8 |
| F1 T1A | in (19,0), out (10,0) | **Row 0** (was row 1); overhangs the bottom edge 1.43 |
| F2 T1.6A | in (19,5), out (10,5) | — |
| 1N4007 | A (11,8), K (14,8) | Unchanged |
| TVS | K (13,9), A (19,9) | Unchanged |
| R-C terminal | **C (21,8), R (23,8)** | **Swapped**, so R sits at the far edge with only empty neighbours |
| 470 µF | − (20,2), + (20,4) | Axis row 3 (y −38.79), leads bent 1.2 from the bung, x −6.39..13.61, glued down |

**Off the board:** both 100 nF, the I2C pull-ups (on the SHT40/OLED module if a probe shows none), the NPN drivers (a strip at the relay input terminal, only if bring-up step 2 fails), and the XL OUT 100 nF (on the XL terminals).

### Cable landings

Every cable wire goes into its own hole from the top and is soldered underneath. Pairs of the same net are twisted and
tinned into one hole (two 26 AWG cores, about 0.8 mm).

| Hole | Wire(s) |
|---|---|
| (0,0) | UP |
| (1,0), (2,0), (3,0) | relay IN1, IN2, IN3 |
| (4,0) | SDA, cover + SHT |
| (5,0) | SCL, cover + SHT |
| (2,6) | 3V3, cover + SHT (via the trough) |
| (5,6) | RIGHT (via the trough) |
| (1,8) | relay GND + XL OUT− |
| (2,8) | cover GND + SHT GND |
| (1,9) | XL OUT+ + relay 5V |
| (3,8) | relay IN4 |
| (4,8), (5,8), (6,8) | LEFT, DOWN, OK |
| (20,6), (21,6) | XL IN+ (bus), XL IN− (GND) |
| (10,9) | COM, 18 AWG (drilled to 1.3) |
| Terminal | R and C field wires |

### Cable routes

All are polylines in the JSON.

- **Relay harness** (6, dressed flat 2 × 3): leaves the top-left corner (−61.5,−21,9.5) for the existing x −63.2 channel.
  IN1–3 come off it into lane L.
- **Lane L** (top side, x −63.9..−60.1, y −44.5..−21, z 7.6..12.8): 11 wires. It runs under the XIAO's USB end, ≥ 1.45
  below the plug envelope (z 14.25).
  - Eight go on round the end of the row-1 header strip (x < −59.66) into the **bottom fan** (y ≈ −47.5) and drop
    into row 0.
  - Three turn into the **trough** at y ≈ −31.5, z 10.4–13.2. That is above the resistor bodies (z ≤ 10) and below
    the XIAO PCB (16.1).
- **Top fan** (y −21.9..−20.5, z 8–10.6): GND, 5A, IN4, LEFT, DOWN and OK drop into rows 8–9.
- **Cover bundle** (15 wires: cover 9 + SHT40 4 + XL OUT 2, dressed flat, y −20.0..−14.6, z 15.7..20.5): along y −17.3,
  dropping at x −61.
- **SHT40:** grommet → (9.8,−40,5) → up the right gutter under the cap overhang → rises at y −26..−17.5 → joins the
  cover bundle. It is no longer under the board.
- **XL7015:** 4 wires.
  - The IN pair goes down the right margin at x 4.2 (0.57 from the terminal, clear of the right fence) and along
    y −31.4 to (20,6) and (21,6).
  - The OUT pair peels off at (14,−14) into the cover bundle.
- **COM:** (10,9) straight up to z 23, then along y to −8 and on to COM1. That is 1.7 above the cover bundle and 7.3
  from the antenna end.
- **Field wires:** R from x 4.0 to column 23, C from x 2.0 to column 21. They don't cross each other or the Y1–W wires.

### Mechanics and cover

- **`PB_PRESS`** = (−37,−48.6), (3.6,−48.6), (−39,−21.6), all r 1.2.
  - Removed: (−62.5,−48.6) (bottom fan), (−46,−21.4) (top fan) and (−36.5,−21.4) (redundant).
  - Hold-down at the left end now comes from the fence lips.
- **`PB_SUPPORTS`:** unchanged.
- **Fences:** existing, plus a **new top-left fence at x −58..−53**.
  - Every fence gets a 0.5 lip (0.35 over the board) at z 7.75..8.4, with a flat underside and a 45° chamfer on top. A
    0.5 overhang prints without supports.
  - The bottom-right lip is cut back to x ≤ 2.0, so it clears the (3.6,−48.6) pin by 0.33.
- **Left rim bumps:** x −65.25..−64.33 at y −46.5..−43.5 and −26.5..−23.5, z 3..7.6. They leave 0.15 to the board edge.
- **USB cut:** `tbox(-ix-3, -ix+1, XIAO_CY-7.5, XIAO_CY+7.5, 14.25, 23.25)`, centre (−68, −36.25, 18.75).
- **`FUSE_Y` = (PB(0,0)[1], PB(0,5)[1]).** The pocket follows: y −52.41..−27.71, 1.0 deep.
- **`CAP_AX` = (PB(20,0)[0]+1.2, PB(20,0)[0]+21.2, PB(0,3)[1]).**

### Overlap check (geocheck.py)

The checker uses flat-ended cylinders and sampled capsules for diagonal parts. Items left out on purpose:
- wires that enter what they serve (field wires/window/terminal, SHT40/grommet);
- bundles that merge;
- fence/support-pad joins.

**Real overlaps: none.**

| Remaining flags | Value | Status |
|---|---|---|
| Cover bundle vs the wall-screw driver path | −2.2 | soft |
| COM vs the wall-screw driver path | −0.8 | soft |
| Round relay-harness model vs the relay module | −0.63 | pre-existing; dress flat |
| XIAO / USB-plug contacts | 0.00 | by design |

**Minimum clearances:**

| Pair | mm |
|---|---|
| PD1–PD2 bodies (touch, insulated) | 0.06 |
| Top-fan wires – fence lips | 0.17 |
| Cap – SHT40 cable | 0.29 |
| Pin (3.6,−48.6) – bottom-right fence | 0.33 |
| Pin (−39,−21.6) – top lip | 0.36 |
| 1N5819 – lane L | 0.36 |
| Cover bundle – pin (−39,−21.6) | 0.40 |
| Fuse holders – cover face (1.4 with the pocket) | 0.40 |
| Lane L / bottom fan – header strips | 0.44 / 0.46 |
| F2 – XL IN+ wire | 0.52 |
| F2 – terminal | 0.57 |
| Terminal – XL wires | 0.66 |
| XL harness – window-grommet flange | 0.74 |
| Top fence – wall-screw head | 0.83 |
| F1 – pin (−37,−48.6) | 0.84 |
| Cap end – sensor grommet | 0.99 |
| Cap end – rib | 1.14 |
| F1–F2 holders | 2.70 |

## 3. Hand-wiring map

The full list is `underside_links` and `hole_map` in the JSON. "Bridge" means a 1-pitch underside link of a tinned lead
offcut or solder. On the power side, use stripped and tinned 18 AWG. Wires are 26 AWG for logic and 18 AWG for power.

**Logic island (underside: 1-pitch bridges only)**

- **UP, SDA, SCL:** (0,0)–(0,1), (4,0)–(4,1), (5,0)–(5,1).
- **IN1–IN3:** each pull-down's signal lead goes through (c,2) and is bent along the underside through (c,1) to (c,0),
  for c = 1, 2, 3. One lead joins landing, pin and pull-down.
- **GND:** (0,6)–(1,6)–(1,7). (3,6)–(3,5)–(2,5)–(1,5)–(1,6). (1,7)–(1,8)–(2,8)–(2,9).
- **3V3:** (2,7)–(2,6).
- **D6 → 1k:** (6,1)–(6,2).
- **RIGHT:** (6,6)–(5,6).
- **5V:** (0,7)–(0,8). **5A:** (0,9)–(1,9).
- **IN4:** (3,7)–(3,8)–(3,9).
- **Keys:** (4,7)–(4,8), (5,7)–(5,8), (6,7)–(6,8).

**Power island (planar; 0 crossings)**

| Net | Links |
|---|---|
| R | Insulated 18 AWG from the terminal R pin tail (23,8), along column 23 and row 0, to (20,0). Bridges (20,0)–(19,0) (F1 in) and (19,0)–(18,0). Bare 18 AWG column 18 (18,0)…(18,5), then (18,5)–(19,5) (F2 in). |
| GND | Bare 18 AWG from the C pin (21,8) down column 21 to (21,2), then to (20,2) (cap −). TVS anode (19,9)–(20,9)–(21,9)–(21,8). XL IN− at (21,6). |
| Bus | TVS cathode lead (13,9)→(14,8), bridge (14,8)–(15,8). Insulated 18 AWG (15,8)→(20,7), bridge (20,7)–(20,6) (XL IN+). Cap + lead through (20,4), **sleeved**, bent to (20,6) past the empty (20,5). |
| F1 out | 1N4007 anode lead **sleeved** from (11,8) down column 11 to (11,0), bridge (11,0)–(10,0). |
| COM | Drilled (10,9). The wire's stripped end is sleeved underside down column 10 and soldered to the F2 pin tail (10,5). |

- **Pads kept empty** (never solder or bridge): (19,1..4), (19,6), (20,1), (20,3), (20,5), (22,*), (17,0..5), (4,6),
  (4,9), (5,9), (6,9), (6,0), (3,2..5) on the top side (PD3 body), and columns 7–9.
- **Adjacency result (netcheck):**
  - 0 R/F1o/bus/24 VAC pads next to anything dangerous;
  - bus–GND pairs only (a bridge blows F1);
  - the remaining relay-on pairs are the two inherent header pairs (3V3–D10, D3–D4) and the added (3,0)–(4,0) IN3–SDA
    pair;
  - every other IN neighbour is GND (fail-safe) or a key line (≈0.6 V).
- **Underside stack:**
  - logic ≤ 1.2 mm: tails flush-cut to ≤ 1.2, bridges lie on their pads;
  - power ≤ about 2.2 mm (one flat 18 AWG; nothing crosses), leaving ≥ 0.8 to the backplate.
- **Pin fit:** if the terminal pins (≈1.1–1.3) don't enter the 1.0 holes, drill (21,8) and (23,8) to 1.3. Lap the
  18 AWG onto the pin tails underneath.

## 4. Build order

1. **Fuse holders.** Solder the header pins to the clip legs with the holder sitting in the board as a jig. Tug-test,
   check ≤ 0.05 Ω, and measure the width (≤ 10.1 is now uncritical) and the installed height (≤ 17.9). Set `FUSE_H`.
2. **Trough parts** (XIAO not fitted, headers not yet on): PD1–PD3 and the 1k, with their leads bent as the
   (c,2)–(c,1)–(c,0) and (6,2)–(6,1) bridges. Then the GND bridges on rows 5–6 and (2,7)–(2,6).
3. **Female headers on rows 1 and 7.** Use a spare XIAO as the jig. Flush-cut every tail to ≤ 1.2.
4. **Top logic parts:** the 1N5819 and PD4 (standing), and the row 7–9 bridges.
5. **Power island:**
   1. Drill (10,9) and, if needed, (21,8)/(23,8).
   2. Fit the 1N4007 (sleeved anode lead), the TVS and the terminal.
   3. Fit the column-18 and column-21 straps and the two insulated 18 AWG links.
   4. Fit the fuse holders and epoxy their bases.
   5. Fit the cap **last**: bent 1.2, + lead sleeved to (20,6), glued.
6. **Ohm test (new bring-up step 0; mandatory without the re-pin):**

   | Measure | Pass |
   |---|---|
   | Each IN to each other IN, 3V3, 5V, 5A, SDA, SCL, D6 | **> 1 MΩ** |
   | Each IN to GND | **10 k** |
   | R to C, R to bus, R to every logic net | open |
   | Bus to C | charges, not 0 Ω |
   | 3V3, 5V, 5A to GND | not shorted |

7. **Bring-up step 1:** power stage alone, XL7015 IN pair only, no XIAO. Set 5.00 V.
8. **Into the case.** The backplate is already on the wall (drive the wall screws **before** dressing the cover bundle
   and COM). Snap the board under the fence lips and route the relay harness flat.
9. **Landings,** with the XIAO unplugged:
   1. Lane L wires to row 0 and the trough first.
   2. Then the top fan.
   3. Then the XL IN pair and the field R/C.
   4. Tie the bundles at the top-left corner.
10. **Fit the XIAO** (spacer removed, PCB at 8.5). Check the USB plug through the cover cut.

## 5. CAD edits (fusion_case_v3.py)

- **Constants:**
  - `XIAO_CY = PB_Y0 + 4*PB_PITCH`
  - `FUSE_Y = (PB(0,0)[1], PB(0,5)[1])`
  - `CAP_AX = (PB(20,0)[0]+1.2, PB(20,0)[0]+21.2, PB(0,3)[1])`
  - `PB_PRESS = [(-37,-48.6), (3.6,-48.6), (-39,-21.6)]`
- **Backplate joins:** the top-left fence, a lip on every fence (bottom-right lip x ≤ 2.0), and two left rim bumps.
- **Cover cuts:** the USB cut becomes 15 × 9 as above.
- **Reference list:**
  - XIAO bodies pin-centred on rows 1/7;
  - the header boxes;
  - PD1/PD2 diagonal, PD3 and the 1k as in the JSON;
  - PD4 and the 1N5819 standing;
  - delete the 100 nF and optional pull-ups;
  - the terminal R/C swap;
  - delete the old XL and COM hole markers.
- **`build_wiring()`:** replace the relay start, cover bundle, SHT40, XL harness (IN pair and OUT pair), COM (z 23) and
  field R/C polylines with the JSON ones.
- **HARDWARE.md controller table:** regenerate it from the JSON.

## 6. Decisions for the user

1. **Optional firmware re-pin.** Not the default. In `board.h`: IN4→D0, UP→D10, SDA→D9, SCL→D8, LEFT→D4, DOWN→D5.
   - **What it buys:** it removes every IN-to-pulled-up pad pair. Both inherent header pairs (D10–3V3, D3–SDA) and the
     added row-0 IN3–SDA pair go away, so a solder bridge can no longer hold W or O on through a boot.
   - **Why I don't default to it:** every one of those bridges is a build defect that the step-0 ohm test catches
     before power. An IN3–SDA bridge also kills I2C, which trips the sensor fault (all outputs off) within 2 min.
   - **The cost:** editing safety-verified firmware (`board.h`, `tools/check_safety_sources.py` expectations), and
     re-running the relay bench checks (reset drop, the bootloader hook on GPIO0, and scoping the INs through a reset)
     plus confirming GPIO0's reset state on the C6.
   - **If you take it,** the layout barely changes: IN4 moves to (0,0) with the relay group, UP to (3,8), SDA/SCL to
     (4,8)/(5,8), and LEFT/DOWN to (4,0)/(5,0). Re-run netcheck.py.
2. **Measure before printing:**
   - the USB face to the D0 pin centre (pin-centred or CAD offset);
   - the fuse-holder installed height;
   - whether the terminal pins fit 1.0 holes.
3. **Antenna fallback** if Thread RSSI is poor: the U.FL port with an FPC antenna on the cover, selected by GPIO14
   (one line in `board_init()`).
