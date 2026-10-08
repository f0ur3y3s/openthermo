# v3 rev 2: synthesis

Ground truth is fusion_case_v3.py. All values are in mm. The arithmetic was checked in scratchpad/r2chk.py.

## Rulings

| # | Item | Ruling | Reason (verified) |
|---|---|---|---|
| 1 | Key-nub preload | **Critic is right. Fix it.** | See the checks below. |
| 2 | Window skirt | **Critic's direction, with a captive collar added.** Proud 0.6, t 0.5, flare 0.6, hinge at z 0.6. | See the checks below. |
| 3 | Membrane slit | **Critic: print solid, knife-cut on install.** | See the checks below. |
| 4 | Zip tunnel | **Regression confirmed. Adopt the critic's raised bridge.** | See the checks below. |
| 5a | Sensor-grommet slit and squeeze | **Neither side as written.** Print without a slit and thread the 4 wires before terminating the second end. SG_SQUEEZE 0.15. | See the checks below. |
| 5b | Rib-notch fit | **Critic: 0.1 per side press.** | See the checks below. |
| 5c | Channel | **Ø3.2.** | 4 × Ø1.3 square-packed needs Ø3.14, which is more than Ø2.8. |
| 5d | Blind engagement | **Critic: add 1.0 chamfers on the slot mouths and lower the block top to 13.** | See the checks below. |
| 6 | Pin holes | **Both agree: open them up.** Entry Ø2.1 with a countersink, far Ø1.8. | See the checks below. |
| 7a | CRADLE_TOP depends on LIP_R | **Decouple it.** | A trap the sympathizer found. |
| 7b | Cover screw elongation | **Rejected.** | The snap bead (±0.2) is the binding limit, so elongating the screw hole alone buys nothing. |
| 7c | Reliefs, flange notch 0.15 | **Leave them.** | Harmless. |
| 7d | KEY_TRAVEL bench check | **Keep.** | Measure how far the lever tip can travel. If it bottoms before 1.75, use KEY_TRAVEL 1.0. |

### 1. Key-nub preload
- **The flaw.** The Ø3 flat nub touches the 16° lever at s = −1.5 (L416, L425). The lever top there is 20.309 + 3.75·11.6/13.1 = **23.629**, against NUB_Z0 23.0, so the real preload is **0.63**.
- **Why the check missed it.** With the 0.3 slab cap at L563 the check volume is **1.216**, which matches the reported 1.22. The uncapped volume is 1.557. The slab thickness hid the problem.
- **Effect.** The key travels only 0.26 before the click.
- **Fix values.** With NUB_R 1.0 and contact at s = −1.0:
  - SW_Z1 = 25 − 1.8 − 3.75·11.1/13.1 = **20.0225**.
  - CAR_Z0 = 12.823, CRADLE_TOP = 20.673, STOP_Z = 21.873.
  - The click comes at 0.65 of key travel (0.48 for a 0.8 tip click, 0.90 for a 1.3 tip click).
  - The tip is deflected 1.65 at the stop. At rest, the tip clears its own flange by 0.66.
- **Clearances after the drop.** The carrier and posts still clear the cap: post (57, −18) is 1.5 clear in x, and the cap at y −21.5 tops out at z 11.4, below the carrier at 12.8. The ghost-press margin becomes 1.9.

### 2. Window skirt
- **The current skirt doesn't fit.** It is 2.4 long (L383) and has to tilt into a 1.8 groove. Its tip lands at about 2.4, so it gets pinched under the land.
- **The tolerance budget is 0.2.** The cover screw has 1.7 vs 1.5 (L317) and the snap bead ±0.2 (L298, L333), so the backplate can't stand off the wall by more than 0.2.
- **Why the critic's shortened skirt isn't enough alone.** Starting the skirt at z 0.6 leaves 0.6 of axial slide, because only the flange retains it. The skirt would retract into the bore and the tip would carry no load. A collar to the groove ceiling removes that slide.
- **New skirt geometry.** A 1.2-long skirt hinged at z 0.6 must reach 60° tilt. Its tip lands at offset about 1.4, inside the 1.8 groove with 0.4 margin.
- **Strain and volume.** Corner hoop strain is about 16%, against 27% for the critic's case. Volume is 0.3 against 1.8 mm² per mm.
- **Groove width stays.** The groove can't be widened because the CTL insert hole edge is at y −21.0 and the groove edge at −20.8.

### 3. Membrane slit
- **Printed open area.** 15.6 + 2 × 3.24 = **22 mm²**, the same as a Ø5.3 hole.

### 4. Zip tunnel
- **What blocks it.** The boss (L348) spans x −53.5..−21.5, z 2.5..4.5 and is in the same union as the bridge (L349–351). It refills x −22.5..−21.5 of the tunnel, leaving a 0.3 slit (z 4.5..4.8).
- **Clearances for the raised bridge.**
  - The top at 8.3 clears the MOD PCB (y ≥ −7) by 1.5 in y.
  - It clears the flange edge at −19.5 by 0.3.
  - It clears the #8 head at −23.19 by 0.69.

### 5. Sensor-cable grommet
- **5a, the printed slit.** A 0.4 slit printed in TPU with a 0.4 nozzle won't survive anyway. Both reviewers already say "clean it with a knife".
  - A squeeze equal to the slit width (0.4 = 0.4) gives zero contact pressure.
  - A solid block with 0.15 per side gives 6% bulk strain, which is a seal with modest slide force.
- **5b, the rib notch.** The current notch has 0.1 clearance per side over z 3..12. That is a leak, and the block lifts out with the cover. A 0.1 per side press fixes both.
- **5d, blind engagement.** The VSKIN tips (z 3.5) currently meet the block top 12.5 before home, while the rim engages at 8. Lowering the block to 13 cuts the blind travel to 1.5. Lead-in capture becomes ±1.7.

### 6. Pin holes
Horizontal holes print about 0.15 short in Z. The far hole as modelled is Ø1.75, which is line-to-line with the filament and would end up as a hard press.

## Change list (fusion_case_v3.py)

**Parameters**
- **L52:** add `NUB_R = 1.0`.
- **L59:** `SW_Z1 = ZF - 1.8 - LEV_LIFT * (LEV_PIVOT + NUB_R) / LEV_L`, which gives 20.0225. The comment should say the contact is at the nub's tip-side edge, s = −NUB_R.
- **L53:** change the comment to "click at about 0.65 of 1.2".
- **L65:** `CRADLE_TOP = SW_Z1 + 0.65`. This is fixed and independent of LIP_R.
- **L79:** change the comment to "1.8 ledge".
- **L80:** `GRM_SKIRT_T, GRM_SKIRT_PROUD, GRM_SKIRT_FLARE = 0.5, 0.6, 0.6`. Add `GRM_COLLAR_Z = 0.6`.
- **L78:** delete GRM_SLIT.
- **L84:** `SG_Z = (PLATE, 13.0)`.
- **L85:** `SG_CH_D = 3.2`.
- **L86:** delete SG_SLIT.
- **L87:** `SG_SQUEEZE = 0.15`.
- **L92:** `PIN_ENTRY_R, PIN_FAR_R = 1.05, 0.90`.

**Keys**
- **L416 and L425:** change the nub radius from `1.5` to `NUB_R`.

**Cover**
- **After L293:** add slot-mouth chamfers with 1.0 legs. For each `(x0, x1, zb)` in `((VDIV_X-DIV_T/2, VDIV_X+DIV_T/2, 10.4), (18.9, 20.1, 3.5))`, and for each `yy` in `(SG_Y[0]+SG_SQUEEZE, SG_Y[1]-SG_SQUEEZE)`:
  - `c = tobox((x0+x1)/2, yy, 1, 0, x1-x0+0.4, 1.414, zb-0.707, zb+0.707)`
  - rotate 45° about X through (0, yy, zb), the same as L404–406;
  - `cuts.append(c)`.

**Backplate**
- **L349:** change the bridge z1 to `PLATE + MNT_BOSS + 3.8`.
- **L350:** change the tunnel z to `(PLATE + MNT_BOSS, PLATE + MNT_BOSS + 1.8)`.
- **L366:** change the seat y to `SG_Y[0] + 0.1, SG_Y[1] - 0.1`. This is a press fit. x and z are unchanged.

**Window grommet**
- **L382–390:** replace with the following.
  - Collar: `U(tube, win_off(0.075, GRM_COLLAR_Z, GRM_GROOVE_D))`.
  - Then `nst = 4`, `z_top, z_bot = GRM_COLLAR_Z, -GRM_SKIRT_PROUD`.
  - In the loop: `o = 0.075 + GRM_SKIRT_FLARE * (k + 1) / nst`. The rest of the loop is unchanged.
- **L393–396:** delete the slit and cross-slit cuts, so the membrane prints solid.

**Sensor block**
- **L401:** delete the slit cut. Keep the 45° lead-ins (L402–407). They follow the new SG_Z.

**Carrier pins**
- **After L479:** add the entry countersink: `slots.append(tcone((hx+lx_*3.95, hy+ly_*3.95, hz), PIN_ENTRY_R, (hx+lx_*4.30, hy+ly_*4.30, hz), PIN_ENTRY_R + 0.35))`.

**Reference lever**
- **L558:** set `nseg = 26`.
- **L562:** evaluate `ztop` at `sa`, the high end of the slab.
- **L563:** set the slab z to `max(SW_Z1, ztop - 1.0) .. ztop`.
- The expected nub interference becomes about 0.18 mm³ per key; the true value is 0.08. The sensor-grommet interference becomes about 10 mm³.

## TPU print orientation
- **Window grommet**
  - Print it flange/membrane-down, so the membrane is 2 solid bed layers. The collar and skirt point up and flare 27°, which needs no supports.
  - Use a single 0.5 outer wall on the skirt and print at 20–30 mm/s.
  - On install, insert it tip-first from the room side.
  - Knife-cut an 8–9 mm slit (or a 6×6 "+") where the cable emerges.
- **Sensor block**
  - Print it on an x end-face, so the footprint is 5 × 10 and the part is 5.8 tall. The Ø3.2 channel is then vertical and round, and the lead-ins are vertical edges.
  - Thread the 4 wires through before soldering the second end.
  - Press it into the rib seat.
  - Knife-cut the slit to the channel only if retrofitting.
