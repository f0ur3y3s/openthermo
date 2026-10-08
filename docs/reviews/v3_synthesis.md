# v3 synthesis: rulings and change list

Ground truth: /home/claude/thermo/fusion_case_v3.py. All mm. Z=0 is the wall. Outline stays 138×114×27, because it covers the old 110×75 Braeburn plate with 14 mm per side horizontally.

The stray rounded block in the screenshot was desk-clock geometry. It is ignored.

## 1. New wall mount (Braeburn 1220NC footprint, bare drywall)

### Where the mount line goes
The mount line is at **y = −13.5**, in the existing channel between the CTL top edge (y −20) and the relay terminal row (y −7). The channel is the only band that is free across the whole width:
- The x=16.5 divider only exists below y −31.7.
- The right column below the carrier (z < 13.5) is open.

The current wire window, trrect(−17, 17, −19, −8), is already centred at (0, −13.5), so it stays as it is. Screws go at x = ±37.5 with ±10 travel, which accepts spacings of 65–85.

| Check | Value |
|---|---|
| Shank slot | x ±25.3..±49.7 |
| Head (#8, Ø8.6) footprint | x ±23.2..±51.8, y −17.8..−9.2 |
| Head to terminals (y ≥ −7) | 2.2 |
| Head to CTL edge (y −20) | 2.2 |
| Head to window (x ±17) | 6.2 |
| Head to MOD posts (y ≥ −5.25) | clear |
| Head to CTL posts (y ≤ −19.5) | clear |
| Ø6 driver shaft at y −13.5 to terminals | 4.5 |
| Ø6 driver shaft to CTL parts (y ≤ −21) | 4.5 |

- **Countersink.** The countersink is 82° and must leave a land under the head. On a 3.0 mm plate a #8 countersink is 2.42 deep, which leaves a 0.58 land. Adding a 1.5 local boss (z 3..4.5) raises the land to 2.08. For comparison, the v3 #6 countersink leaves 0.87.
- **Pan heads.** A pan head sits nearly flush in the cone.
- **Paint coverage.** The case sits centred over the hole horizontally. Vertically it extends 70.5 above the screw line and 43.5 below it. If the old plate was centred on its screws, the plate reaches 37.5 below the line, which leaves 6 mm of margin. Check the paint outline below the screws before printing.

### Install order (decided)
1. Pre-assemble the relay module, CTL board (with F1/F2), XL7015, bulk cap and harness on the bench.
2. Pull the cable through the window.
3. Drive both screws through the channel. Only the cover has to be off.
4. Land the wall wires last, because they lie in the same channel.
5. Stuff foam or putty into the drywall hole around the cable. This is the primary draft seal. The gasket is the secondary seal.

### Third anti-rotation screw: omitted
Two screws 75 apart on a slot already fix rotation. Every far-from-line spot is taken:
- under the module PCB (3 mm gap);
- the XL7015 zone;
- the chamber, which a screw hole would make leaky.

### Wire runs after the change
| Run | Length |
|---|---|
| Window to K1–K4 NO terminals | ≤ 60 |
| Window to R-C terminal | ≈ 12 |
| R-C to F1/F2 left ends | ≈ 25 |
| F2 to K4 COM, up the channel at x 10–14 | ≈ 35 |

No 24 VAC wiring crosses the UI column.

## 2. Ruling table

| Item | Ruling | Reason |
|---|---|---|
| B1 wire window vs gang box | **Superseded** | There is no box. The window stays at x ±17, y −19..−8 and is the mount-line centre. |
| M1 thermal | **Accept, modified** | Move the XL7015 (0.4 W) out of the CTL to the zone under the OLED. It ends up ≥ 50 mm from the chamber and downstream of it. Close the notch, make the divider a cover-hung double skin, and apply a firmware offset per relay state. Reject the critic's mirror of the CTL layout: it puts the XIAO (0.3 W) beside the divider and forces the USB slot to move. Latching relays or an SSR are out of scope. |
| M2 F1 vs K1 entry / holder type | **Accept** | F1 and F2 become inline Ø11×45 holders lying on the CTL perfboard, where the XL7015 was, each held by 2 zip-ties through the perfboard. Panel-mount holders are rejected because there is no room for a Ø12 hole plus 30–45 depth. |
| M3 Dupont height | **Accept** | Desolder the header and solder the wires flat. The top lands at about z 10.6. |
| M4 key rattle / no stop / ghost press | **Accept, refined** | Preload by 0.2 and add stop legs. Travel is set to 1.5, not 1.2, so the lever still has room to click. Ghost-press is confirmed: at a 1.5 press, the up and right flanges reach z 22.7, below the centre lever top at 23.2, and the right flange sits over the down-lever pivot end. Relieve the flanges at those spots. |
| M5 cover harness | **Accept** | 1×9 JST-PH on the CTL with a 60 mm slack loop. The pins are VCC, GND, SDA, SCL, U, D, L, R, C. |
| m1 XL7015 height | **Resolved by move** | Zone B allows 16 mm of body above the plate. **New finding:** the spec says 47×24, but the model reserves 44×16, so the old CTL layout could not have closed anyway. |
| m2 SHT40 slot | **Accept** | The slot was 16 long. It becomes 18.8 for the 18×12 board standing 12 tall (z 6..18). |
| m3 key clearance | **Accept** | Use 0.4 plus a 0.4 stepped lip at the face. |
| m4 flange gap 0.2 | **Accept** | d goes from 0.1 to 0.18, except on the centre-lever diagonal, where d = 2.0. |
| m5 keys proud | **Accept** | Keys stand 1.0 proud. At full travel the cap is still 0.3 above the levers. |
| m6 OLED retention | **Accept, no geometry this rev** | Hot-glue 2 corners for now. Heat-stake pins come after the module holes are measured. |
| m7 switches fall out | **Accept** | A dot of CA. No geometry change. |
| m8 insert SKUs | **Accept** | Everything uses M3×4 short inserts in Ø4.0 holes. CTL posts go up to r3.5. |
| m9 0.07 skin on the side intake | **Accept** | Cut from ix−3. |
| m10 one screw line / thin land | **Accept intent** | Two slots plus 4.5 local bosses. No third hole (see §1). |
| m11 strain relief | **Accept** | Add a zip-tie bridge left of the window. |
| m12 26 mm port bridge | **Accept** | Two 1.2 ribs. |
| m13 LED glow | **Accept** | Kapton. No geometry change. |
| m14 spec text | **Accept** | Fix the L128 comment and the "bays separated" wording. |
| m15 aesthetics | **Reject** | The outline is needed to hide the old plate and paint. |
| S: relay height | **Sympathizer right** | The SRD is 15.5 tall, so the relay top is at 23.1. |
| S2 gang knock-out | **Reject** | Moot, because there is no box. |
| S3 countersink vs rail | **Moot** | The gang holes are removed. The rail now clears the M3 boss by 1.3. |
| S5 flange chamfer | **Reject** | A 1.3 overhang prints fine in PETG, and a chamfer would cost flange bearing area. |
| S6 key wobble | **Merged** | Covered by M4: the 2 legs per key define the rocking line. |
| Gasket | **Accept** | Use a TPU or closed-cell foam frame in a 1.0 rear recess. |

## 3. Change list (fusion_case_v3.py)

### Parameters
- Delete `GANG`.
- Add `MNT_X, MNT_Y, MNT_HALF = 37.5, -13.5, 10.0`.
- Add `MNT_RS, MNT_RH, MNT_BOSS = 2.2, 4.3, 1.5`.
- Set `NUB_Z0 = ZF - 2.0`.
- Key sizes: `R_C_KEY = 4.9`, `R_IN_KEY = 7.3`, `R_OUT_KEY = 15.1`.
- Add `KEY_PROUD = 1.0` and `STOP_Z = 22.0`.

### Helpers
- **`slot_csk(cx, cy, half, zt)`**
  - Make the slot from two `tcyl` of r MNT_RS at cx±half plus a `tbox` between them, all over z −1..zt+1.
  - Add a cone r MNT_RS at zt−2.416 to r MNT_RH+0.01 at zt+0.01, repeated every 0.5 mm along the slot.
- **`sector_up(rin, rout, d, z0, z1, d_ne=None, d_nw=None)`**
  - Use d_ne for normal (−1,1)/√2. This is the up key's NE edge.
  - Use d_nw for normal (1,1)/√2.
- **Comment L128.** Change it to "tangential, keeps the lever out of the F2/slot area".

### Backplate
- **Remove** both `countersunk(GANG…)` calls and the ring at L286-288.
- **Add joins:**
  - Slot bosses: `tbox(s*21.5, s*53.5, -19.5, -7.5, PLATE, PLATE+MNT_BOSS)` for s = ±1, with x sorted.
  - Strain-relief bridge: `tbox(-21, -18, -18.5, -8.5, PLATE, PLATE+4)`, minus the tunnel `tbox(-21.1, -17.9, -16.25, -10.75, PLATE-0.01, PLATE+1.8)`.
  - XL7015 rails: `tbox(16, 61, y-1, y+1, PLATE, PLATE+1.5)` at y = 23.5 and y = 45.5. Each rail gets two zip tunnels, `tbox(x-2.5, x+2.5, y-1.1, y+1.1, PLATE, PLATE+1.0)`, at x = 24 and x = 53.
- **Add cuts:**
  - `slot_csk(±MNT_X, MNT_Y, MNT_HALF, PLATE+MNT_BOSS)`.
  - Gasket recess: `trrect(-24, 24, -27.5, 0.5, -1, 1.0, 4)` minus the islands `tcyl((8,-1.75,-2), (8,-1.75,2), 3)` and `tcyl((11.5,-23,-2), (11.5,-23,2), 3)`.
  - Keep the window cut (L285).
- **CTL inserts.** Post r 3.0 → 3.5. Hole r 1.75 → 2.0.
- **Rail (SHT40).**
  - Outer: `tbox(UX-10.7, UX+10.7, -50.5, -45.5, 1, 9)`.
  - Slot: `tbox(UX-9.4, UX+9.4, -49, -47, 6, 10)`.

### Cover
- **Double skin** (joins, 0.5 above the plate):
  - VSKIN: `tbox(18.9, 20.1, -52.9, -34.9, 3.5, ZF+0.3)` plus `tbox(18.9, 20.1, -iy-0.5, -52.9, 8.5, ZF+0.3)`.
  - HSKIN: `tbox(18.9, 64.9, -36.1, -34.9, 3.5, ZF+0.3)` plus `tbox(64.9, ix+0.2, -36.1, -34.9, 8.5, ZF+0.3)`.
- **Cable notch (L230).** `tbox(VDIV_X-2, VDIV_X+4.6, -36.5, -34.0, 12.0, 14.5)`. Seal it with putty after cabling.
- **Transfer port (L231).** y becomes HDIV_Y−4.2..HDIV_Y+2. Split it into x [UX−13, UX−4.9], [UX−3.7, UX+3.7] and [UX+4.9, UX+13].
- **Side intakes (L239).** x0 becomes `ix-3`.
- **Key holes.** Add a second hole body: `R_C_HOLE+0.4`, `R_IN_HOLE-0.4`, `R_OUT_HOLE+0.4`, `WEB-0.4`, over z DEPTH−0.4..DEPTH+1. This is the elephant-foot lip.

### Keys
- **Caps** span z ZF..DEPTH+KEY_PROUD. Ring-key cap offset is `WEB+0.4`.
- **Ring flanges** use d = 0.18. Exceptions:
  - up uses d_ne = 2.0;
  - right uses d_nw = 2.0 (its NE edge after the −90° rotation).
- **Right-key relief.** After rotating the right key, subtract `tbox(49, 53.5, -17.5, -12.5, FL_Z0-0.1, ZF+0.1)`.
- **Stop legs.** Ø1.6, z STOP_Z..FL_Z0, offsets from DC in up-local coordinates before rotation:
  - up, left, right: (±3.65, 11);
  - down: (−3, 7.35) and (−3, 14.65).
- **Centre-key legs.** At world (39.83, 0.00) and (45.00, −5.17).
- **Travel.** 1.5 to the cradle tops at 20.5. Verify the click at about 1.2.

### Reference bodies
- **Delete:** F1, F2, XL7015 and the 470 µF body.
- **Add:**
  - F1: `tcyl((-35,-46,13.1), (10,-46,13.1), 5.5)`.
  - F2: the same at y −34.5.
  - XL7015: `tbox(14, 63, 21.5, 47.5, 4.5, 16.5)`, with the trim pot toward x > 56.
  - Cap: `tcyl((30,-25.2,8), (52,-25.2,8), 5)`. If the real cap is Ø12.5, recheck it against hrib and the boss.
  - JST: `tbox(-36, -14, -28, -23, 7.6, 13.6)`.
- **Move and resize:**
  - R-C terminal: y −29..−21.
  - Dupont: z mz..mz+3.
  - SHT40: `tbox(UX-9, UX+9, -48.8, -47.2, 6, 18)`.

### Spec and firmware
- Fix the spec wording on the webs (1.6) and the bays.
- Add a firmware temperature offset table keyed on the energised relays.
