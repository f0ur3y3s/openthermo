# Sympathizer review: openthermo controller board (round 4)

Scope: the current layout on the 30 × 70 mm, 10 × 24-hole perfboard with no mounting holes, as defined by
`fusion_case_v3.py` (`PB()`, `CTL`, `FUSE_*`, `CAP_AX`, `PB_SUPPORTS`, `PB_PRESS`, `build_wiring()`, Reference list)
and the HARDWARE.md "Controller board" table. Hole (c, r) means column 0..23 along x and row 0..9 along y. Hole (0,0) is
XIAO D0 at x −58.39, y −46.41. All numbers below come from the script constants and were checked in Python
(`scratchpad/symp/geo.py` and `scratchpad/symp/netmap.py`).

**Verdict:** the layout is sound and well zoned. It puts logic on the left, an empty antenna gap in the middle, and
24 VAC power on the right. It uses every hole the board has without wasting any. Its real weaknesses are small and
local:

- the optional I2C pull-ups, as drawn, don't fit;
- the cross-board wiring is unspecified and would cost six long underside wires;
- the 3 mm gap under the board is tight under the header tails;
- two fit facts need measuring: fuse-holder width, and how high the XIAO actually sits.

None of these needs the layout to move. A concrete hand-wiring map is in section 4.

---

## 1. What the layout gets right, and why

### 1.1 Zoning (safety rule 1 and radio performance, both met by the floor plan itself)

| Zone | Columns | x (mm) | Contents |
|---|---|---|---|
| Logic | 0–6 | −58.4 … −43.2 | XIAO, pull-downs, 1 k, decouplers, 1N5819 |
| Antenna gap | 7–9 | −40.6 … −35.5 | bare board; only plastic (a support pad and a cover pin) at its edge |
| 24 VAC and DC bus | 10–23 | −33.0 … 0.0 | fuses, 1N4007, TVS, 470 µF, R-C terminal, XL landing, COM exit |

- The USB end has to face the left wall, and the antenna sits at the opposite end. So the antenna can only point into
  the board, and the layout gives it the one stretch of board that is empty.
- The 24 VAC parts sit as far right as the board goes. The field wires come into the R-C terminal from the wall
  window, which is 3–11 mm away. No 24 VAC conductor runs left of column 10.
- **Measured keepout.** The XIAO footprint in the CAD ends at x −39.0. From there:
  - fuse-holder plastic (x −34.96): **4.0 mm**;
  - first 24 VAC metal, the fuse pins in column 10 (x −33.0): **6.0 mm**;
  - COM-wire exit (10, 9): about **8.7 mm**.
- **The CAD may be pessimistic here.** If the XIAO is centred on its pin rows, as Seeed's footprint is, its body spans
  x −61.27..−40.27, not −60..−39. That puts the antenna end 1.27 mm further left, and the distances become 5.3, 7.3
  and 9.7 mm. Either way the "5–10 mm, no metal, no 24 VAC" target is met, at its lower bound in the worst case.
- **Cheap fallback.** If Thread RSSI is poor, the XIAO C6 has a U.FL port and an RF switch (GPIO14 picks the
  external antenna). A stick-on FPC antenna on the cover's inner face fixes it without touching the board layout.

### 1.2 The XIAO block is wired by adjacency

- **Pull-downs under the XIAO.** They are 2.4 mm tall in an 8.5 mm gap, which leaves 6.1 mm. Each pull-down's signal
  end sits one hole from its GPIO pin, so a pull-down is physically at its pin. That is the best place for a resistor
  whose job is the tens of milliseconds after power-on.
- **IN1 needs no wiring at all.** D1 is at (1,0) and GND at (1,6) in the same column, so its pull-down drops straight
  across.
- **The 1 k on D6 (U0TXD)** is in column 4, away from the antenna-end column 6.
- **The decouplers** sit in row 7, one hole from the 5V, GND and 3V3 pins.
- **The 1N5819** sits in row 9 with its cathode toward 5V. All of this happens in cols 0–6, rows 7–9, outside the XIAO
  footprint (XIAO top edge y −29.89; row 7 is at y −28.63).
- Nothing under the XIAO gets warm, and the parts there can be serviced by unplugging the XIAO.

### 1.3 The power block packs the hardest parts into the only space that holds them

- **Fuse holders (26.8 × 10 each) lie along x.** That is the board's long axis and the holder's natural PCB
  orientation. The 9-hole pin pitch (22.86 mm) is exactly the measured leg spacing. The pins in cols 10 and 19 use the
  full span between the antenna gap and the capacitor column.
- **Fuse height:** the holders reach z 24.6 against the cover face at 25.0, and the 1 mm fuse pocket in the cover
  makes the clearance **1.4 mm**.
- **Fuse service:** the caps face up (+z), so both fuses come out from the front with the cover off.
- **Lying holders are the only option.** Standing ones don't exist in this holder type, and the 17 mm height
  already uses up the depth.
- **1N4007 (row 8, cols 11–14) and TVS (row 9, cols 13–19)** sit in the strip above F2:
  - F2's top edge to the 1N4007 body: **1.22 mm**.
  - TVS to the board's top edge: **0.82 mm**.
  - The 1N4007 cathode (14,8) and the TVS cathode (13,9) are diagonal neighbours, so the bus node is one bent lead.
- **R-C terminal (cols 21/23, row 8):** its entries face the wall window and its screws face the front.
  - It clears F2 by **0.57 mm** in x.
  - It is as close to the field wires as the board allows, which keeps the 18 AWG R and C short and keeps them away
    from the logic end.

### 1.4 The 470 µF overhang is a feature, not a hack

The cap can't stand up (27.6 mm against a 25 mm cover). Lying on the board, a 10 × 20 can would take about 8 × 4 holes,
roughly the whole right-hand block. Instead its leads sit in column 20 (rows 1 and 3) and the can lies along +x.
**7.59 mm** of it hangs past the board edge into the dead strip between the board (x 5.82) and the divider rib
(x 14.75). No board could use that strip anyway.

| Check | Clearance |
|---|---|
| Can end (x 13.41) to the divider rib (14.75) | **1.34 mm** |
| Can end to the cover's divider tongue (≈15.7) | **2.29 mm** |
| Can to the bottom-right fence (top at z 8.4) | the can's underside is at z ≈ 10.8 where the fence is: clear |
| Can to cover pin (3.6, −48.6) | 1.07 mm in y; the pin is beyond the can's radius |
| SHT40 cable under the can (channel at z 6) | can underside z ≈ 7.8 at y −40: **≈ 0.4 mm** over a soft cable, by design |

- Thermally it is the best spot for an electrolytic: far from the relays (y ≥ −4.5) and from the XL7015, which is up
  under the OLED.

### 1.5 The cradle is the right answer for a board with no holes

- **Seven support pads**, five of them under the 5.8 mm and 3.6 mm hole-free margins. **Fences** stand 0.15 mm off
  the edges. **Five cover pins** stop 0.15 mm above the board top.
- **No hardware, nothing to drill, and it doesn't rattle.** The board lifts out with the cover off.
- **Every pad and pin was checked against the hole grid.** Each hole within reach of a pad or pin footprint is one the
  layout already leaves empty:

  | Support pad or pin | Holes in reach |
  |---|---|
  | Pad and pin at (−37, −48.6) | (8,0), (9,0): the antenna gap |
  | Pad and pin at (−46, −21.4) | (5,9) |
  | Pad at (−21.5, −21.4) | (14,9), (15,9): under the TVS body |
  | Pin at (−36.5, −21.4) | (8,9), (9,9): the antenna gap |

  That only works if those holes stay empty underneath. The map in section 4 keeps them empty.

- **The cover pins only touch bare margin or the antenna gap.** That matches the intent that only plastic sits in front
  of the antenna.

### 1.6 Wires soldered to the XIAO's header tails

The XIAO is socketed, so the header tail is the only place each GPIO can be reached from below. This approach:
- uses no holes;
- gives the shortest path to every pin;
- keeps every off-board wire out of the space above the XIAO, which is the RF side;
- survives an XIAO swap without touching a single joint.

The tail is a stiff 0.64 mm square post, and 26 AWG silicone wire is flexible. This is a standard and sound method. Its
two real costs are strain relief and how the wires stack in the 3 mm gap; both are dealt with in section 3.

### 1.7 Cable routes are short and land in their own zones

| Cable | Route |
|---|---|
| Relay harness | leaves from the top-left corner, straight up the 3 mm channel at x ≈ −63 to the input terminal |
| Cover bundle | arrives over the top-left corner from the right-hand channel, at z 17.5 along y −17.5 (clears the press pin at (−46, −21.4) by 0.2 mm and the terminal top in y by 2 mm) |
| XL7015 cable | lands at the right end, next to the bus and GND it serves |
| F2 → COM | exits at (10, 9), the closest point to relay COM1 (x −51) that is outside the antenna gap; runs at z 21, about 22 mm from the XIAO in y |
| R and C | the shortest possible run, from the window to the terminal |

---

## 2. Where it is genuinely weak, and the smallest fix for each

| # | Weakness | Evidence | Smallest fix that keeps the layout |
|---|---|---|---|
| W1 | **The optional I2C pull-ups don't fit as drawn.** Row 8, cols 0–3 and 3–6, would "share the 3V3 hole in column 3". | Two 1/4 W leads (≈0.6 mm each) won't go into one 1.0 mm hole. (3,8) isn't 3V3: column 3 is D3/D10, and 3V3 is (2,6). The SDA/SCL ends at (0,8) and (6,8) are 20 mm from D4/D5, which sit on row 0. (0,8) also blocks the 1N5819 cathode's straight path to 5V. The pull-up body clears the diode by only 0.04 mm. | **Take them off the board.** The OLED module almost always has 4.7 k pull-ups, and most SHT40 modules have 10 k. If a probe shows neither, solder two 10 k across the OLED's VCC–SCL pads (they are adjacent) and VCC–SDA. That frees row 8 for the landing field in §4. |
| W2 | **The cross-board wiring isn't specified.** | XL 5V lands at col 23 but is needed at the 1N5819 anode (4,9). XIAO GND has to reach C. The SHT40 runs under the board from x 6 to x −48, along y −40. That makes six long underside wires, most of which cross the 24 VAC straps and the fuse-pin tails, and they cross under the antenna gap. Silicone insulation squeezed onto clipped leads in a 3 mm gap is the one credible long-term failure here. | Re-route them; the board stays as it is (§3.2). The SHT40 runs in the **bottom gutter**. XL 5V and a second XL GND ride the **existing cover-bundle route** to the top-left. Result: **zero** long underside wires. |
| W3 | **Header tails in a 3 mm gap.** | Tails of up to 2 mm leave 1.0 mm below them, and a 26 AWG silicone wire is about 1.3 mm OD. Wires can't pass under a tail, only between tails (≈1.2 mm gaps). | Trim the tails under the XIAO block to **≤ 1.2 mm** (flush-cut after soldering), which leaves 1.8 mm. Optional CAD fix: a **1.5 mm deep trough** in the backplate top under x −61..−42, y −48..−23. That gives 4.5 mm locally. It prints with no supports because it is an open pocket on the up-facing surface. Cut it around the support-pad footprints, or the pads lose their base. |
| W4 | **F1 and F2 are 0.16 mm apart** (rows 1 and 5 = 10.16 mm, holders 10.0 wide). | Any holder over 10.16 mm wide collides. | Measure the holder width. If it is over 10.1 mm, move **F1 to row 0**. It then hangs 1.4 mm past the bottom edge into empty gutter. It still clears the cover pin at (−37, −48.6) by 0.84 mm in x and is clear of the bottom fences. The gap between holders becomes 2.7 mm. Change `FUSE_Y[0]` to `PB(0,0)[1]`; the pocket follows. |
| W5 | **USB height depends on how the XIAO is mated.** | The cut is z 15..22.5 around a USB centre of 18.7, which assumes the XIAO PCB sits 8.5 mm up. A normal male header with its 2.5 mm spacer puts it at 11 mm: USB centre 21.2, only 1.3 mm under the top of the cut, so a typical 6–7 mm overmold won't fit. | Build note: solder the male pins through the XIAO, **slide the black spacer off, and trim the pins to about 6 mm**, so the XIAO rests on the female header (8.5 mm). Check it with the actual cable before printing the cover. |
| W6 | **The cap's lead in column 20 is next to F1's R pin (19,1).** | A solder bridge (20,1)–(19,1) shorts R to C unfused (cap −), or bypasses F1 and the diode (cap +). The positions make this adjacency unavoidable. | Inspect it, and do the pre-power ohmmeter checks in §5. A dab of RTV under the can also stops it rocking on its leads. |
| W7 | **Pulling a fuse cap can lift the board** when the cover is off. | The cradle has no hold-down without the cover. | Hold the board down with a finger while pulling a cap. The R/C/COM 18 AWG wires already tether it. Not worth a CAD change. |

---

## 3. Improvements that build on the current layout

### 3.1 Land wires on component tails and straps, not on bare header tails

- **Land each relay IN wire on its pull-down's signal-end tail** in row 1: IN4 at (0,1), IN1 at (1,1), IN2 at (2,1),
  IN3 at (3,1).
  - This spreads the joints and avoids reheating the header joints.
  - More importantly, it **fails safe**. If the strap or jumper between the GPIO and the pull-down breaks, the relay
    input still has its 10 k to GND and stays off. If the wire landed on the header tail instead, a broken link would
    leave IN floating at boot.
- **GND wires** land on the row-5 GND strap or the top-left GND landings.
- **RIGHT** lands on the 1 k's free end (4,5).
- **3V3** lands on (2,6) and on the C3 lead at (2,7), one wire per joint.
- **Two-wire joints remain only on D4 and D5** (SDA/SCL from both the OLED and the SHT40): twist, tin and lap-solder.

### 3.2 Re-route the cross-board wires (fixes W2)

**SHT40, through the bottom gutter.**
- Route: grommet (14.4, −40, z 6) → x ≈ 10, under the can's overhang → south to y ≈ −52.5 → west along the gutter
  between the board edge (−49.98) and the rim (−54.85) → north under the bottom margin at x ≈ −50 → D4 (4,0) and
  D5 (5,0), only 3.6 mm in.
- The gutter is 4.7 mm wide between the bottom fences (x −57.2..−1.2) and 3.5 mm wide south of the right-hand fence.
- The route crosses no strap, no 24 VAC and no tail, and stays outside the board's footprint the whole way. It adds
  about 25 mm of cable.
- SHT GND goes to (3,5), the PD3 GND end; SHT 3V3 goes to (2,6).

**XL 5V, plus an optional XL GND2, over the top.**
- At (4.5, −18), where the XL harness already meets the cover-bundle channel, the 5V wire, plus a second GND from the
  XL7015's OUT− terminal, peel off and follow the cover bundle to the top-left.
- They land top-side at (5,8) for 5V and (3,8) for GND2.
- The 5 V supply and its return then travel as a pair, and the XL harness at column 23 carries only bus+ and GND.
- With GND2 fitted, no cross-board GND is needed: the XIAO end reaches C through the XL7015's common IN−/OUT−, which is
  still C = GND.
- Without GND2, run a single insulated 26 AWG from the GND strap at (21,1) along the clean row-0 underside lane: row 0
  of cols 7–23 has no tails.

### 3.3 Top-side landing field in rows 8–9, freed by W1

These wires enter from the top through the hole and are soldered underneath. The hole gives strain relief, and the
joint can be inspected from the top. Landings:

| Hole | Wire |
|---|---|
| (1,8) | cover GND |
| (2,8) | relay GND |
| (3,8) | XL GND2 (optional) |
| (4,8) | relay 5V |
| (5,8) | XL 5V |

Clearances: from a row-8 wire (1.3 OD) to the 100 nF bodies **0.64 mm**, and to the 1N5819 body **0.49 mm**. The XL
5V wire reaches (5,8) along y ≈ −25.5, over the diode, to stay clear of the press pin at (−46, −21.4). Hole (5,9)
stays empty, because the support pad at (−46, −21.4) sits 0.37 mm from its centre.

### 3.4 Strain relief without metal

- Lace each underside bundle to the board with waxed thread or floss through two free holes near where it exits:
  (4,7)/(5,7) for the relay IN group, and (6,8)/(6,9) for the cover signals.
- Add a dab of hot glue where each bundle crosses the board edge.
- Sleeve bare leads that run over other pads with insulation slid off 18 AWG wire (ID ≈ 1 mm, which fits a 0.8 mm
  diode lead).

### 3.5 Drill the COM exit

Hole (10,9) is a 1.0 mm hole, and 18 AWG (1.02 mm solid, about 1.2 mm stranded) won't pass through it. Drill it to
1.3 mm. Strip 14 mm of the COM wire, push it down through, sleeve the protruding conductor, bend it along column 10 to
the F2-out tail at (10,5), and solder both ends. The COM wire is then its own underside link, with no extra jumper.

### 3.6 Optional: the 1 k to column 6

The 1 k could sit in column 6: (6,1) bridges to D6 at (6,0) and (6,5) takes RIGHT. That removes the (4,1)→(6,0)
jumper and frees column 4, so SDA can land at (4,1). The 1 k would be no further toward the antenna than the column-6
header pins already are. **Only do this if the XIAO C6's chip antenna is confirmed to sit beyond the last pin pair.**
Otherwise keep the CAD's column 4; it costs one 7 mm jumper.

---

## 4. Hand-wiring map (current part positions)

Checked in `netmap.py`: no hole is used by two nets, every pad-adjacent hole stays empty, and 71 of 240 holes are
used. Top view, row 9 at the top.

**Legend:**
- Logic: d0..d10 = XIAO pins and nets; SA/SC = SDA/SCL; Rk = RIGHT key (1 k end); 3V = 3V3.
- Power: 5V = XIAO 5V; 5A = 1N5819 anode (XL 5V and relay 5V); G = GND (C); R = 24 VAC R; f1 = F1 out;
  CM = F2 out/COM; B+ = DC bus.
- Grid: x = antenna gap; . = free.

```
      0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
r9   5V  .  .  . 5A  .  .  x  x  x CM  .  . B+  .  .  .  .  . G   .  .  .  .
r8   5V G  G  G  5A 5A  .  x  x  x  . f1  .  . B+  .  .  .  .  .  . R   . G
r7   5V G  3V G   .  .  .  x  x  x  .  .  .  .  .  .  .  .  .  .  . R   . G
r6   5V G  3V dA d9 d8 d7  x  x  x  .  .  .  .  .  .  .  .  . R  R  R   . G
r5   G  G  G  G  Rk  .  .  x  x  x CM  .  .  .  .  .  .  . R  R   .  . B+ G
r4    .  .  .  .  .  .  .  x  x  x  .  .  .  .  .  .  .  . R   .  .  . B+ G
r3    .  .  .  .  .  .  .  x  x  x  .  .  .  .  .  .  .  . R   . B+ B+ B+ G
r2    .  .  .  .  .  .  .  x  x  x  .  .  .  .  .  .  .  . R   .  .  .  . G
r1   dA d1 d2 d3 d6  .  .  x  x  x f1  .  .  .  .  .  .  . R  R  G  G  G  G
r0   d0 d1 d2 d3 SA SC d6  x  x  x  .  .  .  .  .  .  .  .  .  .  .  .  .  .
```

### Logic block (cols 0–6)

**Bare straps on the underside** (tinned lead offcuts):
- (1,0)–(1,1), (2,0)–(2,1), (3,0)–(3,1): pull-down signal ends to D1, D2 and D3.
- Row 5 (0,5)–(1,5)–(2,5)–(3,5), with a tap (1,5)–(1,6): the pull-down GND bus.
- (0,6)–(0,7)–(0,8)–(0,9): 5V, carrying the C5 lead and the 1N5819 cathode.
- (1,6)–(1,7)–(1,8)–(2,8)–(3,8)–(3,7): GND, carrying the C5 and C3 GND leads and the landings.
- (2,6)–(2,7): 3V3, to C3.
- (4,9)–(4,8)–(5,8): the 1N5819 anode net.

**Insulated 26 AWG jumpers (2):**
- (0,1) → D10 tail (3,6), for the PD4 signal end.
- (4,1) → D6 tail (6,0), for the 1 k. This one goes away with §3.6.

**Wire landings:**

| Cable | Wire | Lands at |
|---|---|---|
| Relay | IN4 | (0,1) |
| Relay | IN1, IN2, IN3 | (1,1), (2,1), (3,1) |
| Relay | GND | (2,8), top side |
| Relay | 5V | (4,8), top side |
| Cover | UP | (0,0) tail |
| Cover | SDA, SCL | (4,0), (5,0) |
| Cover | LEFT, DOWN, OK | tails (4,6), (5,6), (6,6) |
| Cover | RIGHT | (4,5) |
| Cover | GND | (1,8), top side |
| Cover | 3V3 | (2,7) tail |
| SHT40 | SDA, SCL | (4,0), (5,0) |
| SHT40 | GND | (3,5) |
| SHT40 | 3V3 | (2,6) |
| XL | 5V | (5,8), top side |
| XL | GND2 | (3,8), top side |

Keep holes (4,7), (5,7) and (6,7) empty. They act as a buffer, so there is no 5V hole next to a GPIO-key hole.

### Power block (cols 10–23)

**R:** a bare 18 AWG strap (strip the 18 AWG on hand, twist and tin it): (21,8)–(21,7)–(21,6)–(20,6)–(19,6)–(19,5)
for F2-in, then (18,5)–(18,4)–(18,3)–(18,2)–(18,1)–(19,1) for F1-in. It runs down column 18, not 19, so (19,2..4) stay
empty and R is never next to the capacitor's (20,3) lead.

**GND:** a bare 18 AWG strap, (23,8)–(23,7)…(23,1)–(22,1)–(21,1)–(20,1), where (20,1) is cap −. The XL GND lands at
(23,5).

**TVS anode:** the TVS anode lead, sleeved, runs along row 9 from (19,9) to (23,8). It must not be soldered at (21,9),
which sits next to the R pin.

**Bus:**
- The TVS cathode lead bends from (13,9) to the 1N4007 cathode at (14,8).
- An insulated 18 AWG jumper runs from (14,8) to (20,3), cap +.
- A bare strap runs (20,3)–(21,3)–(22,3)–(22,4)–(22,5), and the XL IN+ lands at (22,5).

**F1 out:** the 1N4007 anode lead, sleeved, runs down column 11 from (11,8) to (10,1). It is sleeved because it passes
next to the F2-out pin at (10,5).

**COM:** see §3.5.

**Remaining adjacencies:**
- Bus next to GND at (22,3..5)/(23,3..5). A bridge there only blows F1.
- The unavoidable (19,1) R next to (20,1) cap −; see W6.
- In the logic block, the risky neighbours are only GND or 5V next to a GPIO. A bridge there either forces a relay OFF
  or shorts a rail, and the continuity checks catch both.
- No 24 VAC hole is next to any logic hole.

**Long underside wires:** CAD as drawn, implicitly 6; this map, **0**. Off-board wires at the XIAO end that come in from
under the board: relay IN ×4, cover signals ×7, SHT40 ×4. All of them are short (≤ 20 mm under the board).

---

## 5. Build order and checks

1. **Fuse holders first:** solder the header pins to the clip legs, then tug-test and check each clip to pin is
   ≤ 0.05 Ω. Measure the width (W4) and the installed height (≤ 17.9 mm).
2. **Under-XIAO parts:** 4 × 10 k and the 1 k, with the row-1 and row-5 straps and the two jumpers. Do this before the
   headers, while the underside is uncrowded.
3. **Female headers:** plug a spare XIAO in as a jig, solder one pin at each end, check the rows are square, then solder
   the rest. Flush-trim the tails to ≤ 1.2 mm.
4. **Top-left parts:** C5 and C3 (on the 5V, GND and 3V3 straps), then the 1N5819.
5. **Logic continuity check:**
   - each IN pin reads 10 k to GND;
   - D6 reads 1 k to (4,5);
   - 5V–GND, 3V3–GND and 5V–3V3 are not shorted.
6. **Power block:** the R and GND straps, the 1N4007 and TVS with their leads routed as in §4, and the bus jumper. Drill
   (10,9). Fit the R-C terminal and the fuse holders, and the cap **last**, lying down, with a dab of RTV under the can.
7. **Pre-power ohmmeter checks**, fuses out and then in:
   - R to C is open;
   - R to bus reads as a diode, not 0 Ω;
   - bus to C reads the cap charging, not 0 Ω;
   - R to every logic net is open.
8. **Bring-up step 1** (power stage alone): connect the XL7015 with no XIAO plugged in and set it to 5.00 V. The zoning
   means the power block can be tested before any logic wire lands.
9. **Landings:**
   - relay harness, then cover bundle, then SHT40 via the gutter;
   - lace and glue them;
   - plug in the XIAO (the one with the spacer removed; W5) and check USB cable fit against the cut.

**Removing the XIAO:** cover off, unplug USB, lift the XIAO straight up. Nothing sits above it (the relay module is at
y ≥ −4.5). Every wire stays on the board, so a spare C6 from the 3-pack, pre-flashed, swaps in about 30 s.

---

## 6. For the synthesizer: points to verify rather than argue

- **XIAO C6 pin offset.** Are the pins centred on the 21 mm length (body x −61.27..−40.27) or as in the CAD
  (−60..−39)? Where exactly is the chip antenna relative to the D6/D7 pins? This affects the keepout numbers (4.0 or
  5.3 mm) and §3.6.
- **Fuse-holder width.** The pass/fail point is 10.16 mm.
- **XIAO height over the board.** Is it 8.5 or 11 mm? This decides whether the USB fits the cut.
- **I2C pull-ups.** Do the SHT40 and OLED modules already have them? If either does, the on-board pull-ups disappear
  entirely.
