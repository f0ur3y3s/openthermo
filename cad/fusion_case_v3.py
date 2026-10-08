"""
openthermo enclosure, v3 (landscape): electronics column left; OLED, D-pad and sensor chamber column right.
Frame (mm): origin = enclosure centre on the wall; X right, Y up, Z out of the wall.
Components: Cover, Backplate, D-pad keys (PETG), D-pad switch carrier (PETG), Reference (not printed).
"""
import adsk.core, adsk.fusion, math, traceback

app = adsk.core.Application.get()
des = adsk.fusion.Design.cast(app.activeProduct)
des.designType = adsk.fusion.DesignTypes.ParametricDesignType
root = des.rootComponent
tbm = adsk.fusion.TemporaryBRepManager.get()
P = adsk.core.Point3D.create
V = adsk.core.Vector3D.create
VI = adsk.core.ValueInput.createByReal
BT = adsk.fusion.BooleanTypes
log = []

def mm(v): return v / 10.0

# ------------------------------------------------------------ parameters (mm)
OW, OH, DEPTH = 138.0, 114.0, 27.0       # landscape
R_OUT, CHAMF, WALL, CLR = 10.0, 3.0, 2.0, 0.15   # CLR: rim gap per side. Fit ring at 0.35 printed 0.30/side (too loose); 0.15 -> ~0.1 real
BW, BH = OW - 2 * (WALL + CLR), OH - 2 * (WALL + CLR)
R_BP = R_OUT - WALL - CLR
PLATE, RIM_T, RIM_H, SNAP_Z = 3.0, 1.6, 8.0, 5.0
SNAP_R = CLR + 0.45                           # snap bead radius: 0.45 engagement past the cover's inner wall
SNAP_GROOVE = (SNAP_R + 0.2, SNAP_R + 0.55)   # cover groove: half-height, depth past the inner wall
FIT_RELIEF = (0.4, 0.6)                       # rim foot relief (elephant foot) and cover open-edge lead-in: depth, height
DIV_SLOT, DIV_RIB = 1.15, 1.75                # backplate rib: tongue slot half-width (0.35 per side round the 1.6 tongue), rib half-width
ZF = DEPTH - WALL                         # front inner face, 25
SCREW = (58.0, 7.0)                       # bottom M3 screw x, z
# screws (M2 / M3 only, threaded straight into PETG; no inserts)
M3_PILOT_R, M3_CLEAR_R = 1.3, 1.7          # 2.6 pilot for an M3 machine screw to thread-form, 3.4 clearance
M2_PILOT_R, M2_CLEAR_R = 0.85, 1.2         # 1.7 pilot for M2, 2.4 clearance
UX = 41.0                                 # UI column centre x
# sensor chamber (bottom of the UI column)
VDIV_X, HDIV_Y, DIV_T = 16.5, -32.5, 1.6
# relay module (AEDIKO 4-ch, 73 x 50, holes 67 x 44.5)
MOD = (-62.0, 11.0, -4.5, 45.5)                # measured 73 x 50, centred on the 67 x 44.5 holes
MOD_HOLES = [(-59.0, -1.75), (8.0, -1.75), (-59.0, 42.75), (8.0, 42.75)]
MOD_PCB_Z = 6.0
FUSE_H = 17.0                                  # holder height with the cap; set to the measured installed height
# controller board: a 30 x 70 perfboard with 10 x 24 holes at 2.54 and no mounting holes, held in a cradle. Hole (0, 0) is
# the XIAO's D0 pin, so the XIAO sits on the grid with its USB end at the left wall; the last column of holes is x 0.03.
PB_PITCH = 2.54
PB_X0, PB_Y0 = -58.39, -46.41
def PB(c, r):
    return (PB_X0 + PB_PITCH * c, PB_Y0 + PB_PITCH * r)
CTL = (PB_X0 - 5.79, PB_X0 + 23 * PB_PITCH + 5.79, PB_Y0 - 3.57, PB_Y0 + 9 * PB_PITCH + 3.57)   # x -64.18..5.82, y -49.98..-19.98
XIAO_CY = PB_Y0 + 3 * PB_PITCH                 # XIAO rows on grid rows 0 (D0-D6) and 6 (5V-D7), cols 0-6
# fuse holders lie along x, pins 9 holes apart (cols 10 and 19): F1 on row 1, F2 on row 5. 4 mm of plastic, 6 mm to the
# metal clips, from the XIAO's antenna end (x -39).
FUSE_X = (PB(10, 0)[0] - 1.97, PB(19, 0)[0] + 1.97)
FUSE_Y = (PB(0, 1)[1], PB(0, 5)[1])            # holder centre lines (10 wide)
FUSE_POCKET = (FUSE_X[0] - 1.0, FUSE_X[1] + 1.0, FUSE_Y[0] - 6.0, FUSE_Y[1] + 6.0, 1.0)   # 1.0 deep pocket in the cover (H <= 17.9)
CAP_AX = (PB(20, 0)[0] + 1.0, PB(20, 0)[0] + 21.0, PB(0, 2)[1])   # 470 uF (10 x 20) lying along x, leads in col 20 rows 1/3,
                                                                   # overhanging the board edge; ends at x 13.4
# XL7015 buck (listing: 44 x 16 board; height to the top of the tallest part assumed 12, confirm on arrival)
XL_L, XL_W, XL_H = 44.0, 16.0, 12.0
XL_C = (38.5, 34.5)                            # board centre, lying on two rails under the OLED
XL_TIES = (XL_C[0] + 7.0, XL_C[0] - 5.0)       # zip-tie notches either side of the inductor (board turned: OUT end faces left)
# controller board
CTL_TOP = 7.6
# cradle: support pads under the board edges, low fences round it, and pins hung from the cover that hold it down
PB_SUPPORTS = [(-62.8, -48.6), (-37.0, -48.6), (3.6, -48.6), (-62.8, -21.4), (-46.0, -21.4), (-21.5, -21.4), (4.0, -24.0)]
# (top-edge pads stay clear of the wall-window grommet's flange, x -19.5..19.5 down to y -21.5)
PB_PRESS = [(-62.5, -48.6), (-37.0, -48.6), (3.6, -48.6), (-46.0, -21.4), (-36.5, -21.4)]
# OLED (0.96" SSD1306), header removed, wires soldered to pads
OLED_YB = 22.5
OLED_W, OLED_H = 27.3, 27.8
WIN = (11.85, OLED_YB + 9.9, OLED_YB + 22.6)
# D-pad
DC = (UX, -4.0)
P_KEY = 11.0
R_C_HOLE, R_C_KEY, R_C_FL = 5.3, 4.9, 6.0
R_IN_HOLE, R_OUT_HOLE = 6.9, 15.5
R_IN_KEY, R_OUT_KEY = 7.3, 15.1
R_IN_FL, R_OUT_FL = 6.3, 16.5
WEB = 0.8
# square pad (chosen over the round ring after the coupon test): a plus of four rounded-rectangle arrow keys
# around a rounded-square centre key. The four arrow keys are ONE flexure part (see SFX_* below).
SQ_C_HALF, SQ_R = 4.5, 1.5                      # centre key half-size, cap corner radius
SQ_ARM_HW, SQ_ARM_Y = 4.5, (6.4, 15.6)          # arrow key half-width, span along its axis (centred on the r=11 nub)
SQ_HOLE_CLR = 0.3                               # face opening clearance per side (+0.4 elephant-foot lip at the face)
SQ_FL_OUT, SQ_FL_IN, SQ_FL_C = 1.1, 0.5, 0.8    # flange overhang: arrow sides/outer end, arrow inner end, centre key
SQ_CHEV = (2.0, 1.6, 0.4)                       # engraved chevron: half-base, height, depth
NUB_GAP = 0.15                                # nub clears the resting lever (the old 0.2 preload left keys pressed)
NUB_Z0, FL_Z0 = ZF - 1.8 + NUB_GAP, ZF - 0.8  # lever top under the nub's tip-side edge sits at ZF-1.8
NUB_R = 1.0                                   # key nub radius
KEY_PROUD, KEY_TRAVEL = 1.4, 1.5              # no nubs: the lever tip (z ~23.77) sits 0.43 under the pocket ceiling
                                              # (FL_Z0), so 1.5 key travel moves the tip ~0.83, about the ~0.85 click.
                                              # Coupon keys clicked; if a printed key does not, set FL_Z0 = ZF - 1.0.
                                              # Print the key plate at 0.2 mm layers (0.4 arms).
# square flexure key plate (coupon-tested). Local key frame: a = along the key axis from DC, d = across it.
# Arrow keys hang on two folded arms each, both running the same way (parallelogram), so a key drifts ~0.1 sideways
# as it travels instead of stretching the arms. Everything starts on one plane (SFX_Z0) so it prints face-up with
# no supports. No nubs (they preloaded the switches): each lever bears on the ceiling of a bridged pocket.
SFX_Z0 = NUB_Z0                               # print plane: key undersides, arms, frame
SFX_T, SFX_W = 0.4, 1.0                       # arm thickness (z) and width
SFX_D_IN, SFX_D_OUT = (6.2, 7.2), (7.8, 8.8)  # inner / outer leg bands across the axis (flange edge at 5.6)
SFX_A_FOLD = 9.4                              # fold bar starts here along the axis
SFX_A_TAB = (13.6, 14.6)                      # tab joining the inner leg to the key flange
SFX_FRAME = (19.6, 21.4)                      # square frame ring, half-sizes
SFX_EAR_R, SFX_HOLE_R = 3.6, 2.75             # frame ears round the cover posts (post r 2.5); carrier clamp tubes
# centre key guide (it is not on the flexure and tilted/jammed): two pins under the key run in sockets in two carrier
# columns that stand outside the centre cradle walls; the column tops are also the centre key's stops
# The guides are two lengths of 1.75 filament (printed 1.5 pins could snap): snug in blind holes in the key, sliding
# in sockets in the carrier columns. They sit in the two free corners of the key, either side of the diagonal switch.
CK_GUIDE_S, CK_GUIDE_P = 0.0, 4.53            # along / across the centre switch axis: key corners (+-3.2, -+3.2)
CK_HOLE_R, CK_SOCK_R, CK_COL_R = 0.90, 1.0, 1.5 # key hole (snug, as PIN_FAR_R), carrier socket (slip), column
CK_PIN_LEN = 4.0                              # pin length below the key underside (2.5 engaged at rest)
CK_HOLE_TOP = DEPTH + KEY_PROUD - 1.2         # blind hole top in the key (1.2 skin under the cap face)
CK_PIN_CUT = CK_HOLE_TOP - SFX_Z0 + CK_PIN_LEN  # filament length to cut (~7.9)
SW_L, SW_W, SW_H = 12.8, 5.8, 6.0                 # measured with calipers
# lever (measured): 13.1 long, 3.5 wide, free tip 3.75 above the body top, pivot inset 1.3 from the body end.
# Along the switch axis s (0 = key nub contact): body s -1.4..11.4, lever pivot s 10.1, tip s -3.0.
LEV_L, LEV_W, LEV_LIFT, LEV_PIVOT = 13.1, 3.5, 3.75, 10.1
LEV_AT_NUB = LEV_LIFT * LEV_PIVOT / LEV_L          # lever height above the body at s = 0 (2.89)
SFX_POCKET = (-4.5, 12.0, LEV_W + 1.0)             # lever pocket under each key: s from, s to, width
SW_Z1 = ZF - 1.8 - LEV_LIFT * (LEV_PIVOT + NUB_R) / LEV_L   # body top: lever at the nub's tip-side edge (s=-NUB_R) = ZF-1.8
SW_Z0 = SW_Z1 - SW_H
LEV_TOP = ZF - 1.8
CAR_Z1 = SW_Z0
CAR_Z0 = CAR_Z1 - 1.2
LIP_R, LIP_OVER = 0.6, 0.35                        # snap lips: half-round, hook 0.35 over each body top edge
CRADLE_TOP = SW_Z1 + 0.65                          # cradle wall tops = stop-leg landing (decoupled from the lip radius)
STOP_Z = SFX_Z0 - KEY_TRAVEL                       # tops of the stop pillars on the carrier (key undersides land here)
TIP_TOP = SW_Z0 + 1.3
RIB_R, RIB_BITE = 0.5, 0.1                         # cradle crush ribs: 0.1 interference on the body
SLOT_W = 3.6                                       # terminal opening width in the carrier floor
SLIT_W = 1.6                                       # wire slit from each terminal slot to the carrier edge, so wired
                                                   # switches can be moved between carriers without unsoldering
USE_LIPS = False
# wire-window grommet (TPU 95A): flange inside, tube through the window, a flared skirt that folds into a narrow
# groove on the wall face and presses on the wall around the hole, and a slit membrane the cable pushes through.
WIN_W = (-17.0, 17.0, -19.0, -8.0)                 # wire window in the backplate
WIN_R = 4.0
GRM_TUBE_T = 1.2                                   # tube wall
GRM_FL_W, GRM_FL_T = 2.5, 1.2                      # flange width past the window, thickness (on the plate's inner face)
GRM_MEM_T = 0.4                                    # membrane thickness (two 0.2 layers)
GRM_GROOVE_W, GRM_GROOVE_D = 1.8, 1.2              # groove round the window on the wall face (1.8 ledge: no supports)
GRM_SKIRT_T, GRM_SKIRT_PROUD, GRM_SKIRT_FLARE = 0.5, 0.6, 0.6
GRM_COLLAR_Z = 0.6                                 # captive collar behind the window ledge; skirt hinges here
# sensor-cable grommet (TPU 95A): a block that sits on the backplate where the cable crosses the chamber's double
# wall (divider + skin). The cover's walls carry a slot open at their free edge, so the cover slides down over the
# block and squeezes it shut. The cable lies in a channel; knife-cut a slit from the block top down to the channel
# and press the cable in (the cover's squeeze closes it).
SG_X, SG_Y, SG_Z = (14.6, 20.4), (-42.5, -37.5), (PLATE, 13.0)
# SHT40 module (measured 12.56 x 10.5, 2.85 hole in a corner, VIN GND SCL SDA on the far edge), lying flat in the
# chamber, sensor side up, pin edge toward the cable grommet; held by one M2 x 6 into a standoff, resting on two posts
SHT_X0, SHT_Y1 = 40.0, -40.0                   # pin-edge x, +Y edge: centred in the chamber flow, ~20 from the
                                               # divider skin, 14 from the outer wall, under the exhaust port, and the
                                               # standoff clears the bottom screw boss (x 53+)
SHT_L, SHT_W, SHT_T, SHT_Z = 12.56, 10.5, 1.6, 14.0  # along x (pins -> hole), along y, thickness, underside height
                                                     # (mid-depth: in the vent airflow, 11 mm off the backplate)
SHT_HOLE = (SHT_X0 + SHT_L - 2.4, SHT_Y1 - 2.7)       # 2.85 hole centre
SG_CH_Z, SG_CH_D = 6.0, 3.6                        # cable channel (4 x 26 AWG silicone, OD 1.3-1.4: needs ~3.4); low, so
                                                   # the cable passes under the 470 uF's overhang to the board
SG_SQUEEZE = 0.15                                  # per side: wall slot narrower than the block
# switch mounting holes (measured): through the body side to side, 0.5 from the pin face to the hole edge,
# one 2.0 round, one 2.1 x 2.0 slot, 6.5 apart and centred on the body (s 1.75 and 8.25).
HOLE_Z = 0.5 + 1.0                                 # hole centre above the body bottom
PIN_D = 1.75                                       # retaining pin = a length of 1.75 filament
PIN_ENTRY_R, PIN_FAR_R = 1.05, 0.90                # slip fit in the entry wall (countersunk), snug in the far wall
# one pin per switch, through whichever hole has a clear straight path from outside the cluster:
# (hole s, side): side +1 = enter from the (-uy, ux) side of the switch axis
PINS = {'up': (8.25, 1), 'right': (8.25, 1), 'left': (8.25, -1), 'down': (8.25, -1), 'center': (1.75, 1)}
POSTS = [(UX - 14.1, 10.1), (UX + 14.1, 10.1), (25.0, -20.0), (57.0, -18.0)]
# wall mount: Braeburn 1220NC footprint, two screws side by side ~75 apart, wire hole between
MNT_X, MNT_Y = 40.0, -13.5                    # old Braeburn screws (measured 80 apart, wire hole centred between)
MNT_HALF, MNT_VHALF = 8.0, 3.5               # left slot horizontal (spacing 72..88), right slot vertical (levels +-3.5)
MNT_RS, MNT_RH, MNT_BOSS = 2.2, 4.3, 1.5      # #8 shank / head radius, local boss

# ------------------------------------------------------------ temp-BRep helpers
def tbox(x0, x1, y0, y1, z0, z1):
    c = P(mm((x0 + x1) / 2), mm((y0 + y1) / 2), mm((z0 + z1) / 2))
    obb = adsk.core.OrientedBoundingBox3D.create(c, V(1, 0, 0), V(0, 1, 0), mm(x1 - x0), mm(y1 - y0), mm(z1 - z0))
    return tbm.createBox(obb)

def tobox(cx, cy, ux, uy, L, W, z0, z1):
    n = math.hypot(ux, uy); ux, uy = ux / n, uy / n
    obb = adsk.core.OrientedBoundingBox3D.create(P(mm(cx), mm(cy), mm((z0 + z1) / 2)), V(ux, uy, 0), V(-uy, ux, 0),
                                                 mm(L), mm(W), mm(z1 - z0))
    return tbm.createBox(obb)

def tcone(p1, r1, p2, r2):
    return tbm.createCylinderOrCone(P(*map(mm, p1)), mm(r1), P(*map(mm, p2)), mm(r2))

def tcyl(p1, p2, r): return tcone(p1, r, p2, r)

def U(a, b): tbm.booleanOperation(a, b, BT.UnionBooleanType); return a
def D(a, b): tbm.booleanOperation(a, b, BT.DifferenceBooleanType); return a
def I(a, b): tbm.booleanOperation(a, b, BT.IntersectionBooleanType); return a

def rot(body, deg, cx, cy):
    m = adsk.core.Matrix3D.create()
    m.setToRotation(math.radians(deg), V(0, 0, 1), P(mm(cx), mm(cy), 0))
    tbm.transform(body, m)
    return body

def trrect(x0, x1, y0, y1, z0, z1, r):
    b = tbox(x0 + r, x1 - r, y0, y1, z0, z1)
    U(b, tbox(x0, x1, y0 + r, y1 - r, z0, z1))
    for cx in (x0 + r, x1 - r):
        for cy in (y0 + r, y1 - r):
            U(b, tcyl((cx, cy, z0), (cx, cy, z1), r))
    return b

def slot_csk(cx, cy, half, zt, vertical=False):
    """#8 screw slot (along X, or along Y if vertical) with an 82 deg countersink at the top face zt."""
    ux, uy = (0.0, 1.0) if vertical else (1.0, 0.0)
    b = tcyl((cx - ux * half, cy - uy * half, -1), (cx - ux * half, cy - uy * half, zt + 1), MNT_RS)
    U(b, tobox(cx, cy, ux, uy, 2 * half, 2 * MNT_RS, -1, zt + 1))
    U(b, tcyl((cx + ux * half, cy + uy * half, -1), (cx + ux * half, cy + uy * half, zt + 1), MNT_RS))
    depth = (MNT_RH - MNT_RS) / math.tan(math.radians(41))
    n = int(2 * half / 0.5)
    for i in range(n + 1):
        px, py = cx - ux * half + ux * i * 0.5, cy - uy * half + uy * i * 0.5
        U(b, tcone((px, py, zt - depth), MNT_RS, (px, py, zt + 0.01), MNT_RH + 0.01))
    return b

def sector_up(rin, rout, d, z0, z1, d_ne=None, d_nw=None):
    """Ring sector centred on +Y around the D-pad centre, bounded by the two diagonals offset by d."""
    cx, cy = DC
    a = tcyl((cx, cy, z0), (cx, cy, z1), rout)
    D(a, tcyl((cx, cy, z0 - 1), (cx, cy, z1 + 1), rin))
    s2 = math.sqrt(2)
    for (nx, ny), dd in (((-1 / s2, 1 / s2), d_ne), ((1 / s2, 1 / s2), d_nw)):
        dd = d if dd is None else dd
        bx, by = cx + nx * (dd + 25), cy + ny * (dd + 25)
        I(a, tobox(bx, by, -ny, nx, 100, 50, z0 - 1, z1 + 1))
    return a

ANG = {'up': 0, 'left': 90, 'down': 180, 'right': -90}


def sq_shape(name, grow, z0, z1, inner=None):
    """Square-pad key outline grown by `grow` (the inner end of an arrow key grown by `inner` if given)."""
    if name == 'center':
        h = SQ_C_HALF + grow
        return trrect(DC[0] - h, DC[0] + h, DC[1] - h, DC[1] + h, z0, z1, max(0.2, SQ_R + grow))
    gi = grow if inner is None else inner
    b = trrect(DC[0] - SQ_ARM_HW - grow, DC[0] + SQ_ARM_HW + grow, DC[1] + SQ_ARM_Y[0] - gi, DC[1] + SQ_ARM_Y[1] + grow,
               z0, z1, max(0.2, SQ_R + grow))
    return rot(b, ANG[name], *DC)


def sq_chevron(name, z0, z1):
    """Engraved chevron (triangle pointing outward) on an arrow key."""
    hb, ht, _ = SQ_CHEV
    cy = sum(SQ_ARM_Y) / 2 - ht / 2
    t = tbox(DC[0] - hb, DC[0] + hb, DC[1] + cy, DC[1] + cy + ht, z0, z1)
    for sx in (-1, 1):
        ux, uy = -sx * hb, ht
        n = math.hypot(hb, ht)
        nx, ny = -sx * ht / n, -hb / n
        mx, my = DC[0] + sx * hb / 2, DC[1] + cy + ht / 2
        I(t, tobox(mx + nx * 5, my + ny * 5, ux, uy, 30, 10, z0 - 1, z1 + 1))
    return rot(t, ANG[name], *DC)


def sfx_box(name, a0, a1, d0, d1, z0, z1):
    """Box in a key's local frame (a along its axis, d across), mapped round DC."""
    return rot(tbox(DC[0] + d0, DC[0] + d1, DC[1] + a0, DC[1] + a1, z0, z1), ANG[name], *DC)


def ck_guides():
    """Centre key guide positions: either side of the centre switch axis."""
    (ax, ay), (ux, uy) = SWITCHES['center']
    return [(DC[0] + ax + ux * CK_GUIDE_S - uy * sd * CK_GUIDE_P, DC[1] + ay + uy * CK_GUIDE_S + ux * sd * CK_GUIDE_P)
            for sd in (1, -1)]


def sfx_key(name):
    """Flat-bottomed key from the print plane: cap, flange, lever pocket; no nub, no stop legs (on the carrier)."""
    zt = DEPTH + KEY_PROUD
    k = sq_shape(name, 0.0, ZF, zt)
    if name == 'center':
        U(k, sq_shape(name, SQ_FL_C, SFX_Z0, ZF))
        for gx, gy in ck_guides():                                       # blind holes for the filament guide pins
            D(k, tcyl((gx, gy, SFX_Z0 - 1), (gx, gy, CK_HOLE_TOP), CK_HOLE_R))
    else:
        U(k, sq_shape(name, SQ_FL_OUT, SFX_Z0, ZF, inner=SQ_FL_IN))
        D(k, sq_chevron(name, zt - SQ_CHEV[2], zt + 1))
    D(k, sw_box(name, SFX_POCKET[0], SFX_POCKET[1], SFX_POCKET[2], SFX_Z0 - 1, FL_Z0))
    return k

# switch placements: actuation point (local to DC), unit vector tip->pivot
SWITCHES = {
    'up':     ((0.0, P_KEY), (0.0, 1.0)),
    'right':  ((P_KEY, 0.0), (1.0, 0.0)),
    'left':   ((-P_KEY, 0.0), (-1.0, 0.0)),
    'down':   ((0.0, -P_KEY), (1.0, 0.0)),          # tangential, keeps the lever out of the slot/channel area
    'center': ((0.0, 0.0), (1 / math.sqrt(2), 1 / math.sqrt(2))),
}

def sw_box(name, s0, s1, w, z0, z1):
    """Box along a switch axis: from s0 to s1 (relative to the actuation point), width w."""
    (ax, ay), (ux, uy) = SWITCHES[name]
    gx, gy = DC[0] + ax, DC[1] + ay
    sm = (s0 + s1) / 2
    return tobox(gx + ux * sm, gy + uy * sm, ux, uy, s1 - s0, w, z0, z1)

# ------------------------------------------------------------ feature helpers
def new_comp(name):
    occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
    occ.component.name = name
    return occ.component

def rect_body(comp, w, h, height, r):
    sk = comp.sketches.add(comp.xYConstructionPlane)
    sk.sketchCurves.sketchLines.addCenterPointRectangle(P(0, 0, 0), P(mm(w / 2), mm(h / 2), 0))
    ext = comp.features.extrudeFeatures.addSimple(sk.profiles.item(0), VI(mm(height)),
                                                  adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
    body = ext.bodies.item(0)
    edges = adsk.core.ObjectCollection.create()
    for e in body.edges:
        a, b = e.startVertex.geometry, e.endVertex.geometry
        if abs(a.x - b.x) < 1e-6 and abs(a.y - b.y) < 1e-6:
            edges.add(e)
    fi = comp.features.filletFeatures.createInput()
    fi.edgeSetInputs.addConstantRadiusEdgeSet(edges, VI(mm(r)), True)
    comp.features.filletFeatures.add(fi)
    return body

def planar_face_at_z(body, z):
    for f in body.faces:
        if f.geometry.surfaceType == adsk.core.SurfaceTypes.PlaneSurfaceType:
            n = f.geometry.normal
            if abs(abs(n.z) - 1) < 1e-6 and abs(f.pointOnFace.z - mm(z)) < 1e-5:
                return f
    raise Exception('no planar face at z=%s' % z)

def add_bodies(comp, named):
    bf = comp.features.baseFeatures.add()
    bf.startEdit()
    for name, t in named:
        b = comp.bRepBodies.add(t, bf)
        if name:
            b.name = name
    bf.finishEdit()
    coll = adsk.core.ObjectCollection.create()
    for b in bf.bodies:
        coll.add(b)
    return coll

def combine(comp, target, temps, op):
    tools = add_bodies(comp, [(None, t) for t in temps])
    ci = comp.features.combineFeatures.createInput(target, tools)
    ci.operation = op
    ci.isKeepToolBodies = False
    comp.features.combineFeatures.add(ci)

JOIN = adsk.fusion.FeatureOperations.JoinFeatureOperation
CUT = adsk.fusion.FeatureOperations.CutFeatureOperation

# ------------------------------------------------------------ wiring (reference only: routes and bundle sizes)
# Off-board wiring between the parts. Bundles are drawn as one tube sized for their wire count; each wire fans out to
# its terminal at the end. Perfboard-internal wiring is not drawn. Field wires 18 AWG thermostat cable, the rest 26 AWG.
R_FIELD, R_SIG = 0.8, 0.65
def r_bundle(n, r=R_SIG):
    return r * 1.15 * math.sqrt(n)

def wire_path(pts, r):
    b = None
    pts = [q for i, q in enumerate(pts) if i == 0 or math.dist(q, pts[i - 1]) > 1e-6]
    for a, c in zip(pts[:-1], pts[1:]):
        seg = tcyl(a, c, r)
        b = seg if b is None else U(b, seg)
    for q in pts[1:-1]:
        U(b, tbm.createSphere(P(mm(q[0]), mm(q[1]), mm(q[2])), mm(r)))
    return b

def build_wiring():
    w = new_comp('Wiring (not printed)')
    named = []
    zt = MOD_PCB_Z + 1.6 + 5.0                                     # relay screw-terminal entry height
    # relay output terminals (12-way, front face y = my0+0.5): per relay NO, COM, NC from left (check your board)
    mx0, mx1, my0, my1 = MOD
    kx = [(mx0 + mx1) / 2 + dx for dx in (-25.5, -8.5, 8.5, 25.5)]
    yf = my0 + 0.5 - 0.6
    # 1. field wires from the wall window: R, C to the R-C terminal; Y1, G, O, W to NO1..NO4
    wy, wz = -6.5, 13.0
    for i, (nm, k) in enumerate((('Y1', 0), ('G', 1), ('O', 2), ('W', 3))):
        x0 = -6.0 + i * 1.8
        x1 = kx[k] - 5.0
        named.append(('Field %s' % nm, wire_path([(x0, -13.5, 0.5), (x0, -13.5, wz), (x0, wy - i * 0.0, wz),
                                                   (x1, wy, wz), (x1, yf, wz)], R_FIELD)))
    for nm, x0, c in (('R', 1.5, 21), ('C', 6.5, 23)):            # into the R-C terminal's top-facing entries
        x = PB(c, 0)[0]
        named.append(('Field %s' % nm, wire_path([(x0, -13.5, 0.5), (x0, -13.5, 14.0), (x, -18.5, 14.0),
                                                  (x, -21.9, 12.0)], R_FIELD)))
    # 2. relay commons: F2 (left end) -> COM1, then COM1-COM2-COM3-COM4 jumpers
    hc = PB(10, 9)                                                 # F2 out (col 10, row 5) -> underside -> hole (10, 9)
    named.append(('F2 -> COM1', wire_path([(hc[0], hc[1], CTL_TOP + 0.8), (hc[0], hc[1], 21.0), (hc[0], -8.0, 21.0),
                                           (kx[0], -8.0, 21.0), (kx[0], -8.0, 17.0), (kx[0], yf, zt + 1.5)], R_FIELD)))
    for a, c in zip(kx[:-1], kx[1:]):
        named.append(('COM jumper', wire_path([(a + 0.6, yf, zt + 1.5), (a + 0.6, yf - 2.0, zt + 3.5),
                                               (c - 0.6, yf - 2.0, zt + 3.5), (c - 0.6, yf, zt + 1.5)], R_FIELD)))
    # 3. relay harness (5V, GND, IN1-IN4): controller board -> left channel -> input terminal (far edge)
    rb = r_bundle(6)
    cx = mx0 - 1.2                                                 # channel between the module and the rim
    yb = my1 + 3.0
    # wires solder to the XIAO's header tails underneath and leave from under the board's top-left corner
    named.append(('Relay harness (6)', wire_path([(-61.0, -18.0, 4.9), (cx, -18.0, CTL_TOP + rb + 2.0),
                                                  (cx, yb, CTL_TOP + rb + 2.0), (mx0 + 26.0, yb, CTL_TOP + rb + 2.0)], rb)))
    for i in range(6):
        tx = mx0 + 3.5 + i * 5.0
        named.append(('Relay harness fan %d' % (i + 1), wire_path([(mx0 + 1.0 + i * 4.0, yb, CTL_TOP + rb + 2.0),
                                                                    (tx, my0 + 46.2, zt)], R_SIG)))
    # 4. XL7015: one 3-wire cable from the controller board (channel right of the relay module)
    xb = r_bundle(3)
    xc = 14.5
    xl0, xl1 = XL_C[0] - XL_L / 2, XL_C[0] + XL_L / 2
    yl0, yl1 = XL_C[1] - XL_W / 2, XL_C[1] + XL_W / 2
    xx = 14.0                                                      # XL7015 bundle runs low, under the cover harness
    zb = CTL_TOP + xb + 0.2
    # 3 wires: bus +, GND, 5 V out (IN- and OUT- are the same node on the XL7015). Board turned so OUT faces the cable.
    hx_ = PB(23, 0)[0]                                             # lands in col 23, rows 4-5, right of the R-C terminal
    named.append(('XL7015 harness (3)', wire_path([(hx_, -35.0, zb), (4.5, -35.0, zb), (4.5, -18.0, zb), (xx, -17.0, zb), (xx, -12.0, 6.0),
                                                   (xx, yl0 + 1.5, 6.0)], xb)))
    for nm, yy in (('OUT- (GND)', yl0 + 1.5), ('OUT+ (5 V)', yl1 - 1.5)):
        named.append(('XL7015 %s' % nm, wire_path([(xx, yl0 + 1.5, 6.0), (xx, yy, 6.0), (xl0 + 1.0, yy, PLATE + 3.6)], R_SIG)))
    named.append(('XL7015 IN+ (bus)', wire_path([(xx, yl0 + 1.5, 6.0), (xx, yl1 + 1.8, 6.0), (xl1 + 0.5, yl1 + 1.8, 6.0),
                                                 (xl1 + 0.5, yl1 - 1.5, 6.0), (xl1 - 1.0, yl1 - 1.5, PLATE + 3.6)], R_SIG)))
    # 5. SHT40: chamber -> grommet channel -> controller board (right end)
    sy = (SG_Y[0] + SG_Y[1]) / 2
    named.append(('SHT40 (4)', wire_path([(SHT_X0 + 1.1, SHT_Y1 - SHT_W / 2, SHT_Z + SHT_T + 1.2),
                                          (SHT_X0 - 1.5, SHT_Y1 - SHT_W / 2, SHT_Z + SHT_T + 1.2), (SHT_X0 - 4.0, sy, SG_CH_Z),
                                          (SG_X[1] + 1.5, sy, SG_CH_Z),
                                          (SG_X[0] - 0.2, sy, SG_CH_Z), (6.0, sy, 4.5)], r_bundle(4))))
    # (under the 470 uF's overhang, then flat under the board to the XIAO's header tails)
    # 6. cover side: OLED (4) and D-pad (6) pigtails end in JST-PH pairs lying flat under the carrier (x 22..48,
    #    y 5..17); ~80 mm of cover-side slack is stowed there too. Controller side: one 10-wire bundle down the
    #    channel right of the relay module, then high along the board's top edge (over the field wires, under the
    #    F2 -> COM wire) to the XIAO end, where the wires go under the board to the XIAO's header tails.
    cb = r_bundle(10)
    pz = 6.85                                                      # mid-height of the mated pairs
    named.append(('OLED (4)', wire_path([(UX, OLED_YB + OLED_H - 2.0, ZF - 2.6), (UX, OLED_YB + OLED_H + 1.5, ZF - 4.0),
                                         (UX, OLED_YB + OLED_H + 1.5, 12.0), (xc, OLED_YB + OLED_H + 1.5, 12.0),
                                         (xc, 21.0, 12.0), (43.0, 21.0, pz), (43.0, 17.0, pz)], r_bundle(4))))
    named.append(('D-pad (6)', wire_path([(DC[0], DC[1] - 5.0, 9.5), (DC[0] - 5.0, DC[1] - 5.0, 9.5), (20.0, DC[1] - 5.0, 9.5),
                                          (20.0, -6.0, 6.0), (20.0, 18.5, 6.0), (29.0, 18.5, pz), (29.0, 17.0, pz)], r_bundle(6))))
    named.append(('Controller side (10)', wire_path([(29.0, 5.0, pz), (29.0, 2.5, pz), (25.0, 2.5, 10.3), (15.5, 2.5, 10.3), (xc, 0.0, 13.0), (xc, -15.0, 15.0),
                                                     (10.0, -17.5, 17.5), (-57.0, -17.5, 17.5), (-57.0, -17.5, 10.2)], cb)))
    named.append(('OLED pair -> bundle', wire_path([(43.0, 5.0, pz), (43.0, 2.5, pz), (29.5, 2.5, pz)], r_bundle(4))))
    add_bodies(w, named)
    for i, (n, _) in enumerate(named):
        w.bRepBodies.item(i).name = n
    try:
        lib = app.materialLibraries.itemByName('Fusion Appearance Library')
        red = des.appearances.itemByName('Plastic - Glossy (Red)') or \
            des.appearances.addByCopy(lib.appearances.itemByName('Plastic - Glossy (Red)'), 'Plastic - Glossy (Red)')
        for b in w.bRepBodies:
            b.appearance = red
    except Exception:
        pass
    log.append('wiring ok')


MINE = ('Cover', 'Backplate', 'Gasket (TPU)', 'D-pad keys (PETG)', 'D-pad switch carrier (PETG)', 'Reference (not printed)',
        'Wiring (not printed)')

def build():
    # safety: only rebuild in a document that holds nothing but this case
    others = [o.component.name for o in root.occurrences if o.component.name not in MINE]
    if others or root.bRepBodies.count:
        raise Exception('active document holds other work (%s); open the thermostat document first' % others)
    for i in range(root.occurrences.count - 1, -1, -1):
        root.occurrences.item(i).deleteMe()
    ix, iy = OW / 2 - WALL, OH / 2 - WALL          # cover inner half-extents

    # ============================================================ COVER
    cov = new_comp('Cover')
    body = rect_body(cov, OW, OH, DEPTH, R_OUT)
    body.name = 'Cover'
    edges = adsk.core.ObjectCollection.create()
    for e in planar_face_at_z(body, DEPTH).edges:
        edges.add(e)
    ch = cov.features.chamferFeatures.createInput2()
    ch.chamferEdgeSets.addEqualDistanceChamferEdgeSet(edges, VI(mm(CHAMF)), True)
    cov.features.chamferFeatures.add(ch)
    faces = adsk.core.ObjectCollection.create()
    faces.add(planar_face_at_z(body, 0))
    si = cov.features.shellFeatures.createInput(faces, False)
    si.insideThickness = VI(mm(WALL))
    cov.features.shellFeatures.add(si)

    joins = []
    joins.append(tbox(VDIV_X - DIV_T / 2, VDIV_X + DIV_T / 2, -iy - 0.5, HDIV_Y + DIV_T / 2, 10.4, ZF + 0.3))
    joins.append(tbox(VDIV_X - DIV_T / 2, ix + 0.2, HDIV_Y - DIV_T / 2, HDIV_Y + DIV_T / 2, 10.4, ZF + 0.3))
    fx0, fx1 = UX - OLED_W / 2 - 0.15, UX + OLED_W / 2 + 0.15
    fy0, fy1 = OLED_YB - 0.15, OLED_YB + OLED_H + 0.15
    frame = tbox(fx0 - 1.2, fx1 + 1.2, fy0 - 1.2, fy1 + 1.2, ZF - 5.0, ZF + 0.2)
    D(frame, tbox(fx0, fx1, fy0, fy1, ZF - 6, ZF + 1))
    D(frame, tbox(UX - 6.0, UX + 6.0, fy1 - 1, fy1 + 2, ZF - 6, ZF - 2))
    for y in (OLED_YB + 7, OLED_YB + 21):
        U(frame, tbox(fx1 - 0.3, fx1, y - 1, y + 1, ZF - 5, ZF))
        U(frame, tbox(fx0, fx0 + 0.3, y - 1, y + 1, ZF - 5, ZF))
    joins.append(frame)
    for px, py in POSTS:
        joins.append(tcyl((px, py, CAR_Z1), (px, py, ZF + 0.3), 2.5))
    # double-skin chamber walls (1.6 mm air gap to the divider), hung from the cover
    joins.append(tbox(18.9, 20.1, -52.4, -34.9, 3.5, ZF + 0.3))                # (stops 0.65 short of the rim)
    joins.append(tbox(18.9, 20.1, -iy - 0.5, -52.4, 8.5, ZF + 0.3))
    joins.append(tbox(18.9, 64.4, -36.1, -34.9, 3.5, ZF + 0.3))
    joins.append(tbox(64.4, ix + 0.2, -36.1, -34.9, 8.5, ZF + 0.3))
    for px, py in PB_PRESS:                       # hold the controller board down onto its support pads (0.15 gap)
        joins.append(tcyl((px, py, CTL_TOP + 0.15), (px, py, ZF + 0.3), 1.2))
    combine(cov, body, joins, JOIN)

    cuts = []
    cuts.append(tbox(SG_X[0] + 0.4, SG_X[1] + 0.2, SG_Y[0] + SG_SQUEEZE, SG_Y[1] - SG_SQUEEZE,
                     0.0, SG_Z[1]))                                                   # slot for the sensor-cable grommet
    for x0, x1, zb in ((VDIV_X - DIV_T / 2, VDIV_X + DIV_T / 2, 10.4), (18.9, 20.1, 3.5)):     # 1 mm chamfers on the slot mouths
        for yy in (SG_Y[0] + SG_SQUEEZE, SG_Y[1] - SG_SQUEEZE):
            ch = tobox((x0 + x1) / 2, yy, 1, 0, x1 - x0 + 0.4, 1.414, zb - 0.707, zb + 0.707)
            m = adsk.core.Matrix3D.create()
            m.setToRotation(math.radians(45), V(1, 0, 0), P(0, mm(yy), mm(zb)))
            tbm.transform(ch, m)
            cuts.append(ch)
    for x0, x1 in ((UX - 13, UX - 4.9), (UX - 3.7, UX + 3.7), (UX + 4.9, UX + 13)):       # chamber exhaust port, ribbed
        cuts.append(tbox(x0, x1, HDIV_Y - 4.2, HDIV_Y + 2, 16.0, 21.0))
    gh, gd = SNAP_GROOVE
    for x0, x1 in ((ix - 0.1, ix + gd), (-ix - gd, -ix + 0.1)):                           # snap grooves
        cuts.append(tbox(x0, x1, -20.5, 20.5, SNAP_Z - gh, SNAP_Z + gh))
    lead = trrect(-ix - FIT_RELIEF[0], ix + FIT_RELIEF[0], -iy - FIT_RELIEF[0], iy + FIT_RELIEF[0], -1.0, FIT_RELIEF[1],
                  R_OUT - WALL + FIT_RELIEF[0])                                            # lead-in step on the open edge
    D(lead, trrect(-ix, ix, -iy, iy, -2.0, FIT_RELIEF[1] + 1, R_OUT - WALL))
    cuts.append(lead)
    for cx in list(range(-58, 11, 5)) + list(range(22, 62, 5)):                           # exhaust, top
        cuts.append(trrect(cx - 1, cx + 1, iy - 1, iy + 3, 9.0, 20.0, 0.9))
    for cx in list(range(-56, 11, 5)) + list(range(21, 51, 4)):                           # intake, bottom
        cuts.append(trrect(cx - 1, cx + 1, -iy - 3, -iy + 1, 7.0, 20.0, 0.9))
    for cy in (-50.0, -46.0, -42.0, -38.0):                                               # chamber intake, right side
        cuts.append(tbox(ix - 3, ix + 3, cy - 1, cy + 1, 9.0, 20.0))
    cuts.append(tbox(-ix - 3, -ix + 1, XIAO_CY - 6.5, XIAO_CY + 6.5, 15.0, 22.5))          # USB-C, left wall
    cuts.append(trrect(UX - WIN[0], UX + WIN[0], WIN[1], WIN[2], ZF - 1, DEPTH + 1, 1.0))  # OLED window
    for nm in ('center',) + tuple(ANG):                                                  # square-pad key openings
        cuts.append(sq_shape(nm, SQ_HOLE_CLR, ZF - 1, DEPTH + 1))
        cuts.append(sq_shape(nm, SQ_HOLE_CLR + 0.4, DEPTH - 0.4, DEPTH + 1))           # elephant-foot relief
    for px, py in POSTS:
        cuts.append(tcyl((px, py, CAR_Z1 - 0.1), (px, py, CAR_Z1 + 6), M2_PILOT_R))     # M2 x 6 carrier screws
    cuts.append(tcyl((SCREW[0], -iy - 3, SCREW[1]), (SCREW[0], -iy + 2, SCREW[1]), M3_CLEAR_R))
    fpx0, fpx1, fpy0, fpy1, fpd = FUSE_POCKET
    cuts.append(trrect(fpx0, fpx1, fpy0, fpy1, ZF - 1.0, ZF + fpd, 1.0))                 # fuse-holder pocket (opens up when printed)
    combine(cov, body, cuts, CUT)
    log.append('cover ok')

    # ============================================================ BACKPLATE
    bp = new_comp('Backplate')
    cup = rect_body(bp, BW, BH, RIM_H, R_BP)
    faces = adsk.core.ObjectCollection.create()
    faces.add(planar_face_at_z(cup, RIM_H))
    si = bp.features.shellFeatures.createInput(faces, False)
    si.insideThickness = VI(mm(RIM_T))
    bp.features.shellFeatures.add(si)
    cup.name = 'Backplate'
    hx, hy = BW / 2, BH / 2
    joins = [trrect(-hx + 1, hx - 1, -hy + 1, hy - 1, 0, PLATE, R_BP - 1)]
    for sx in (1, -1):
        joins.append(tcyl((sx * hx, -19.0, SNAP_Z), (sx * hx, 19.0, SNAP_Z), SNAP_R))
    vrib = tbox(VDIV_X - DIV_RIB, VDIV_X + DIV_RIB, -hy + 1, HDIV_Y + DIV_RIB, 0.5, 12.0)
    D(vrib, tbox(VDIV_X - DIV_SLOT, VDIV_X + DIV_SLOT, -hy, HDIV_Y + 2, 10.0, 13.0))
    hrib = tbox(VDIV_X - DIV_RIB, hx - 0.5, HDIV_Y - DIV_RIB, HDIV_Y + DIV_RIB, 0.5, 12.0)
    D(hrib, tbox(VDIV_X - DIV_SLOT, hx, HDIV_Y - DIV_SLOT, HDIV_Y + DIV_SLOT, 10.0, 13.0))
    joins += [vrib, hrib]
    for x, y in MOD_HOLES:
        joins.append(tcyl((x, y, 1.0), (x, y, MOD_PCB_Z), 3.5))
    cx0, cx1, cy0, cy1 = CTL                                                     # controller board cradle
    for x, y in PB_SUPPORTS:                                                     # pads under the hole-free edge margins
        joins.append(tcyl((x, y, PLATE - 0.5), (x, y, CTL_TOP - 1.6), 1.8))
    g_, t_, zt_ = 0.15, 1.2, CTL_TOP + 0.8                                       # fences: no overhangs, prints as is
    joins.append(tbox(cx0 + 1.0, cx0 + 7.0, cy0 - g_ - t_, cy0 - g_, PLATE - 0.5, zt_))      # bottom edge, both ends
    joins.append(tbox(cx1 - 7.0, cx1 - 1.0, cy0 - g_ - t_, cy0 - g_, PLATE - 0.5, zt_))
    joins.append(tbox(-45.0, -40.0, cy1 + g_, cy1 + g_ + t_, PLATE - 0.5, zt_))       # top edge, clear of the wall screw
    joins.append(tbox(cx1 + g_, cx1 + g_ + t_, cy0 + 1.0, cy0 + 4.0, PLATE - 0.5, zt_))   # right edge, below the cap
    joins.append(tbox(cx1 + g_, cx1 + g_ + t_, cy1 - 7.0, cy1 - 3.0, PLATE - 0.5, zt_))
    hx_, hy_ = SHT_HOLE                                                          # SHT40 standoff + two rest posts
    joins.append(tcyl((hx_, hy_, PLATE - 0.5), (hx_, hy_, SHT_Z), 2.0))      # thin: less wall heat into the sensor
    for px_, py_ in ((SHT_X0 + 4.5, SHT_Y1 - 1.5), (SHT_X0 + 4.5, SHT_Y1 - SHT_W + 1.5)):
        joins.append(tcyl((px_, py_, PLATE - 0.5), (px_, py_, SHT_Z), 1.2))
    joins.append(tbox(SCREW[0] - 5, SCREW[0] + 4.5, -hy + 1.0, -43.0, 1.0, 11.0))   # (clear of the cover's inner corner)
    bm = MNT_RH + 1.7                                                              # wall-screw slot bosses
    joins.append(tbox(-MNT_X - MNT_HALF - bm, -MNT_X + MNT_HALF + bm, MNT_Y - bm, MNT_Y + bm, PLATE - 0.5, PLATE + MNT_BOSS))
    joins.append(tbox(MNT_X - bm, MNT_X + bm, MNT_Y - MNT_VHALF - bm, MNT_Y + MNT_VHALF + bm, PLATE - 0.5, PLATE + MNT_BOSS))
    br = tbox(-22.5, -19.8, -18.5, -8.5, PLATE - 0.5, PLATE + MNT_BOSS + 3.8)         # zip-tie strain-relief bridge
    D(br, tbox(-22.6, -19.7, -16.25, -10.75, PLATE + MNT_BOSS, PLATE + MNT_BOSS + 1.8))
    joins.append(br)
    for y in (XL_C[1] - 6.5, XL_C[1] + 6.5):                                       # XL7015 rails (under the OLED)
        rl = tbox(XL_C[0] - 22.5, XL_C[0] + 22.5, y - 1, y + 1, PLATE - 0.5, PLATE + 1.5)
        for x in XL_TIES:                                                          # zip-tie notches between parts
            D(rl, tbox(x - 2.5, x + 2.5, y - 1.1, y + 1.1, PLATE - 1.5, PLATE + 0.5))   # 2.0 tunnel, 1.0 roof
        joins.append(rl)
    combine(bp, cup, joins, JOIN)
    cuts = []
    for x, y in MOD_HOLES:                                                         # M3 x 6 into the bosses
        cuts.append(tcyl((x, y, 1.0), (x, y, MOD_PCB_Z + 0.1), M3_PILOT_R))
    cuts.append(tcyl((SCREW[0], -hy - 1.0, SCREW[1]), (SCREW[0], -45.0, SCREW[1]), M3_PILOT_R))   # M3 x 10 closure (2 mm past the tip)
    cuts.append(tbox(VDIV_X - DIV_RIB, VDIV_X + DIV_RIB, HDIV_Y - DIV_RIB, HDIV_Y + DIV_RIB, 10.0, 13.0))   # rib junction groove
    cuts.append(trrect(*WIN_W, -1.0, PLATE + 1.0, WIN_R))                          # wire window
    cuts.append(tbox(SG_X[0] - 0.1, SG_X[1] + 0.1, SG_Y[0] + 0.1, SG_Y[1] - 0.1, PLATE, 13.0))   # grommet seat in the rib (0.1/side press fit)
    gw = GRM_GROOVE_W                                                              # skirt groove on the wall face
    cuts.append(trrect(WIN_W[0] - gw, WIN_W[1] + gw, WIN_W[2] - gw, WIN_W[3] + gw, -1.0, GRM_GROOVE_D, WIN_R + gw))
    cuts.append(slot_csk(-MNT_X, MNT_Y, MNT_HALF, PLATE + MNT_BOSS))                 # left: horizontal
    cuts.append(slot_csk(MNT_X, MNT_Y, MNT_VHALF, PLATE + MNT_BOSS, vertical=True))  # right: vertical
    cuts.append(tcyl((SHT_HOLE[0], SHT_HOLE[1], SHT_Z - 5.5), (SHT_HOLE[0], SHT_HOLE[1], SHT_Z + 1), M2_PILOT_R))  # M2 x 6
    foot = trrect(-hx - 1, hx + 1, -hy - 1, hy + 1, -1.0, FIT_RELIEF[1], R_BP + 1)          # rim foot relief: eats the
    D(foot, trrect(-hx + FIT_RELIEF[0], hx - FIT_RELIEF[0], -hy + FIT_RELIEF[0], hy - FIT_RELIEF[0],  # elephant foot
                   -2.0, FIT_RELIEF[1] + 1, R_BP - FIT_RELIEF[0]))
    cuts.append(foot)
    combine(bp, cup, cuts, CUT)
    log.append('backplate ok')

    # ============================================================ TPU GROMMET
    grm = new_comp('Gasket (TPU)')
    def win_off(o, z0, z1):
        return trrect(WIN_W[0] - o, WIN_W[1] + o, WIN_W[2] - o, WIN_W[3] + o, z0, z1, max(0.3, WIN_R + o))
    tube = win_off(-0.05, GRM_GROOVE_D, PLATE)                       # snug in the window
    U(tube, win_off(GRM_FL_W, PLATE, PLATE + GRM_FL_T))               # flange on the plate's inner face
    for cx, cy in ((11.5, -23.0), (8.0, -1.75)):                     # clear the neighbouring screw posts
        D(tube, tcyl((cx, cy, PLATE - 1), (cx, cy, PLATE + GRM_FL_T + 1), 3.8))
    U(tube, win_off(0.075, GRM_COLLAR_Z, GRM_GROOVE_D))              # captive collar behind the window ledge
    nst = 4                                                          # skirt: stepped flare from the collar to the wall
    z_top, z_bot = GRM_COLLAR_Z, -GRM_SKIRT_PROUD
    for k in range(nst):
        za = z_top - (z_top - z_bot) * (k + 1) / nst
        zb = z_top - (z_top - z_bot) * k / nst
        o = 0.075 + GRM_SKIRT_FLARE * (k + 1) / nst
        ringk = win_off(o, za, zb)
        D(ringk, win_off(o - GRM_SKIRT_T, za - 1, zb + 1))
        U(tube, ringk)
    D(tube, win_off(-GRM_TUBE_T, z_bot - 1, PLATE + GRM_FL_T + 1))    # cable opening
    mem = win_off(-GRM_TUBE_T + 0.01, PLATE + GRM_FL_T - GRM_MEM_T, PLATE + GRM_FL_T)
    # membrane printed solid: knife-cut an 8-9 mm slit (or a 6 x 6 cross) where the cable comes through
    U(tube, mem)
    sg = tbox(SG_X[0], SG_X[1], SG_Y[0], SG_Y[1], SG_Z[0], SG_Z[1])
    yc_sg = (SG_Y[0] + SG_Y[1]) / 2
    D(sg, tcyl((SG_X[0] - 1, yc_sg, SG_CH_Z), (SG_X[1] + 1, yc_sg, SG_CH_Z), SG_CH_D / 2))
    for yy in (SG_Y[0], SG_Y[1]):                                      # 45-degree lead-in on the top edges
        lead = tobox((SG_X[0] + SG_X[1]) / 2, yy, 1, 0, SG_X[1] - SG_X[0] + 2, 1.2, SG_Z[1] - 0.6, SG_Z[1] + 0.6)
        m = adsk.core.Matrix3D.create()
        m.setToRotation(math.radians(45), V(1, 0, 0), P(0, mm(yy), mm(SG_Z[1])))
        tbm.transform(lead, m)
        D(sg, lead)
    add_bodies(grm, [('Grommet', tube), ('Sensor cable grommet', sg)])
    log.append('grommet ok')

    # ============================================================ D-PAD KEYS (square flexure)
    keys = new_comp('D-pad keys (PETG)')
    z0, za = SFX_Z0, SFX_Z0 + SFX_T
    kb = {nm: sfx_key(nm) for nm in tuple(ANG) + ('center',)}
    fx = None
    for nm in ANG:
        k = tbm.copy(kb[nm])
        for sg in (1, -1):
            def band(d):
                return (d[0], d[1]) if sg > 0 else (-d[1], -d[0])
            di, do = band(SFX_D_IN), band(SFX_D_OUT)
            U(k, sfx_box(nm, SFX_A_TAB[0], SFX_A_TAB[1], *band((SQ_ARM_HW + 0.5, SFX_D_IN[0] + 0.01)), z0, za))  # tab
            U(k, sfx_box(nm, SFX_A_FOLD, SFX_A_TAB[1], di[0], di[1], z0, za))                                  # inner leg
            U(k, sfx_box(nm, SFX_A_FOLD, SFX_A_FOLD + SFX_W, *band((SFX_D_IN[0], SFX_D_OUT[1])), z0, za))       # fold bar
            U(k, sfx_box(nm, SFX_A_FOLD, SFX_FRAME[0] + 0.3, do[0], do[1], z0, za))                             # outer leg
        fx = k if fx is None else U(fx, k)
    h0, h1 = SFX_FRAME
    frame = tbox(DC[0] - h1, DC[0] + h1, DC[1] - h1, DC[1] + h1, z0, ZF)
    D(frame, tbox(DC[0] - h0, DC[0] + h0, DC[1] - h0, DC[1] + h0, z0 - 1, ZF + 1))
    for px, py in POSTS:                                                   # ears with post holes, tied to the ring
        ear = tcyl((px, py, z0), (px, py, ZF), SFX_EAR_R)
        dx, dy = px - DC[0], py - DC[1]
        r = math.hypot(dx, dy)
        ux, uy = dx / r, dy / r
        reach = h0 / max(abs(ux), abs(uy)) - r + 0.5
        U(ear, tobox(px + ux * reach / 2, py + uy * reach / 2, ux, uy, reach, 1.6 * SFX_EAR_R, z0, ZF))
        kx, ky = DC[0] + math.copysign(h1, dx), DC[1] + math.copysign(h1, dy)          # fill to the ring corner
        U(ear, tbox(min(px, kx), max(px, kx), min(py, ky), max(py, ky), z0, ZF))
        U(frame, ear)
    I(frame, tbox(DC[0] - h1, DC[0] + h1, DC[1] - h1, DC[1] + h1, z0 - 1, ZF + 1))
    for px, py in POSTS:
        D(frame, tcyl((px, py, z0 - 1), (px, py, ZF + 1), SFX_HOLE_R))
    U(fx, frame)
    add_bodies(keys, [('Flexure key plate', fx), ('Key center', kb['center'])])
    log.append('keys ok')

    # ============================================================ SWITCH CARRIER
    car = new_comp('D-pad switch carrier (PETG)')
    plate = tcyl((DC[0], DC[1], CAR_Z0), (DC[0], DC[1], CAR_Z1), 9.0)
    walls, slots = [], []
    for name in SWITCHES:
        U(plate, sw_box(name, -2.75, 12.75, SW_W + 2.7, CAR_Z0, CAR_Z1))
        (ax, ay), (ux, uy) = SWITCHES[name]
        for side in (-1, 1):
            px, py = -uy * side * (SW_W / 2 + 0.75), ux * side * (SW_W / 2 + 0.75)
            sm = (-1.4 + 12.75) / 2
            gx, gy = DC[0] + ax + ux * sm + px, DC[1] + ay + uy * sm + py
            walls.append(tobox(gx, gy, ux, uy, 12.75 + 1.4, 1.2, CAR_Z1, CRADLE_TOP))
        walls.append(sw_box(name, 11.55, 12.75, SW_W + 2.7, CAR_Z1, CRADLE_TOP))
        walls.append(sw_box(name, -2.65, -1.45, 4.0, CAR_Z1, TIP_TOP))
        # snap lips: half-round beads along the top inner edge of each side wall, flat underside 0.05 above
        # the body top so the switch clicks in past them and cannot lift out; kept clear of the end wall so
        # the walls can flex, and clear of the 3.5-wide lever (lip inner edge 2.55 from the axis vs 1.75)
        lc = SW_W / 2 + 0.15 + LIP_R - (0.15 + LIP_OVER)
        lz = SW_Z1 + 0.05
        for sa, sb in (((-0.5, 3.0), (4.5, 8.0)) if USE_LIPS else ()):
            for side in (-1, 1):
                pa = (DC[0] + ax + ux * sa - uy * side * lc, DC[1] + ay + uy * sa + ux * side * lc, lz)
                pb = (DC[0] + ax + ux * sb - uy * side * lc, DC[1] + ay + uy * sb + ux * side * lc, lz)
                bead = tcyl(pa, pb, LIP_R)
                I(bead, sw_box(name, sa - 1, sb + 1, SW_W + 4, lz, lz + LIP_R + 0.01))
                walls.append(bead)
        # crush ribs: two per side wall, one on the end wall (pushes the body onto the tip stop)
        rc = SW_W / 2 + 0.15 + RIB_R - (0.15 + RIB_BITE)
        for s_rib in (2.0, 8.5):
            for side in (-1, 1):
                rx = DC[0] + ax + ux * s_rib - uy * side * rc
                ry = DC[1] + ay + uy * s_rib + ux * side * rc
                walls.append(tcyl((rx, ry, CAR_Z1), (rx, ry, CRADLE_TOP), RIB_R))
        se = 11.55 + RIB_R - (0.15 + RIB_BITE)
        for off in ((0.0,) if name == 'down' else (-1.9, 1.9)):   # split either side of the wire slit
            ex_, ey_ = DC[0] + ax + ux * se - uy * off, DC[1] + ay + uy * se + ux * off
            walls.append(tcyl((ex_, ey_, CAR_Z1), (ex_, ey_, CRADLE_TOP), RIB_R))
        # floor opening for the switch terminals: wide enough to pass pre-soldered joints (<= ~3.2 mm),
        # leaving 1.1 mm ledges each side and 0.2 at each end for the body to sit on
        slots.append(sw_box(name, -1.2, 11.2, SLOT_W, CAR_Z0 - 1, CAR_Z1 + 1))
        # wire slit, full height (floor and walls), so a wired switch's leads slide out sideways: arrows straight
        # out through the cradle end wall; the centre one angles off the diagonal to miss the post clamp tube;
        # the tangential down switch exits through its -y side wall between the crush rib and the pin hole
        if name == 'center':
            ax0, ay0 = DC[0] + ax + ux * 10.5, DC[1] + ay + uy * 10.5
            bx0, by0 = DC[0] + ax + ux * 22.0 - uy * 8.0, DC[1] + ay + uy * 22.0 + ux * 8.0
            dx_, dy_ = bx0 - ax0, by0 - ay0
            slots.append(tobox((ax0 + bx0) / 2, (ay0 + by0) / 2, dx_, dy_, math.hypot(dx_, dy_), SLIT_W,
                               CAR_Z0 - 1, ZF))
        elif name == 'down':                                   # tangential: exit sideways (-y), between ribs and pillars
            sx_ = DC[0] + ax + ux * 5.5
            slots.append(tbox(sx_ - SLIT_W / 2, sx_ + SLIT_W / 2, DC[1] + ay - 9.0, DC[1] + ay, CAR_Z0 - 1, ZF))
        else:
            slots.append(sw_box(name, 10.5, 16.0, SLIT_W, CAR_Z0 - 1, ZF))
        # retaining pin: through both cradle walls and the switch's mounting hole
        ps, pside = PINS[name]
        hx, hy, hz = DC[0] + ax + ux * ps, DC[1] + ay + uy * ps, SW_Z0 + HOLE_Z
        lx_, ly_ = -uy * pside, ux * pside
        reach = SW_W / 2 + 0.15 + 1.2 + 0.6
        if name == 'center':
            reach = 7.0                              # straight through the centre key's guide columns on both sides
        slots.append(tcyl((hx, hy, hz), (hx + lx_ * reach, hy + ly_ * reach, hz), PIN_ENTRY_R))
        slots.append(tcone((hx + lx_ * 3.95, hy + ly_ * 3.95, hz), PIN_ENTRY_R,
                           (hx + lx_ * 4.30, hy + ly_ * 4.30, hz), PIN_ENTRY_R + 0.35))   # entry countersink
        slots.append(tcyl((hx, hy, hz), (hx - lx_ * reach, hy - ly_ * reach, hz), PIN_FAR_R))
    # fill the wedges between the cradle footprints: one solid plate = convex hull of the cradles + centre disc
    pts = [(DC[0] + 9.0 * math.cos(math.radians(a)), DC[1] + 9.0 * math.sin(math.radians(a))) for a in range(0, 360, 15)]
    for name in SWITCHES:
        (ax, ay), (ux, uy) = SWITCHES[name]
        hw = (SW_W + 2.7) / 2
        for s_c in (-2.75, 12.75):
            for side in (-1, 1):
                pts.append((DC[0] + ax + ux * s_c - uy * side * hw, DC[1] + ay + uy * s_c + ux * side * hw))
    pts = sorted(set((round(x, 4), round(y, 4)) for x, y in pts))
    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    hull_pts = lower[:-1] + upper[:-1]                 # counter-clockwise
    hull = tbox(DC[0] - 60, DC[0] + 60, DC[1] - 60, DC[1] + 60, CAR_Z0, CAR_Z1)
    for i in range(len(hull_pts)):
        (x0, y0), (x1, y1) = hull_pts[i], hull_pts[(i + 1) % len(hull_pts)]
        ex, ey = x1 - x0, y1 - y0
        el = math.hypot(ex, ey)
        nx, ny = -ey / el, ex / el                     # inward normal
        mx, my = (x0 + x1) / 2 + nx * 50, (y0 + y1) / 2 + ny * 50
        I(hull, tobox(mx, my, ex, ey, 300, 100, CAR_Z0 - 1, CAR_Z1 + 1))
    U(plate, hull)
    for px, py in POSTS:
        U(plate, tcyl((px, py, CAR_Z0), (px, py, CAR_Z1), SFX_EAR_R))   # same r as the clamp tube: no overhang lip
        dx, dy = px - DC[0], py - DC[1]
        U(plate, tobox(DC[0] + dx / 2, DC[1] + dy / 2, dx, dy, math.hypot(dx, dy), 5.0, CAR_Z0, CAR_Z1))
        slots.append(tcyl((px, py, CAR_Z0 - 1), (px, py, CAR_Z1 + 1), M2_CLEAR_R))
    pillars = []                                               # stop pillars (the centre key stops on its guide columns)
    for gx, gy in ck_guides():                                 # centre key guide columns with pin sockets
        walls.append(tcyl((gx, gy, CAR_Z1 - 0.01), (gx, gy, STOP_Z), CK_COL_R))
        # socket bottom 0.4 below the pressed pin tip, so it stays above the centre switch's retaining-pin hole
        slots.append(tcyl((gx, gy, STOP_Z - CK_PIN_LEN - 0.4), (gx, gy, STOP_Z + 1), CK_SOCK_R))
    for nm in ANG:
        legs = ((-3.0, 7.35), (-3.0, 14.65)) if nm == 'down' else ((-3.65, 11.0), (3.65, 11.0))
        th = math.radians(ANG[nm])
        for lx, ly in legs:
            pillars.append((DC[0] + lx * math.cos(th) - ly * math.sin(th), DC[1] + lx * math.sin(th) + ly * math.cos(th)))
    for x, y in pillars:                                       # r 0.8 on a 1.2 wall: 45 deg cone base, no overhang
        pl = tcyl((x, y, CRADLE_TOP - 0.3), (x, y, STOP_Z), 0.8)
        U(pl, tcone((x, y, CRADLE_TOP - 0.5), 0.6, (x, y, CRADLE_TOP - 0.3), 0.8))
        walls.append(pl)
    for px, py in POSTS:                                       # tubes round the posts clamp the flexure frame ears
        t = tcyl((px, py, CAR_Z1), (px, py, SFX_Z0), SFX_EAR_R)
        for nm in SWITCHES:                                    # merge into the cradle walls; keep only the switch
            D(t, sw_box(nm, -1.45, 11.55, SW_W + 0.3, CAR_Z1 - 1, ZF))   # cavity and the pin entry path clear
            (ax, ay), (ux, uy) = SWITCHES[nm]
            ps, pside = PINS[nm]
            hx_, hy_ = DC[0] + ax + ux * ps, DC[1] + ay + uy * ps
            lx_, ly_ = -uy * pside, ux * pside
            D(t, tcyl((hx_, hy_, SW_Z0 + HOLE_Z), (hx_ + lx_ * 20, hy_ + ly_ * 20, SW_Z0 + HOLE_Z), PIN_ENTRY_R + 0.5))
        walls.append(t)
        slots.append(tcyl((px, py, CAR_Z1), (px, py, ZF), SFX_HOLE_R))
    for w in walls:
        U(plate, w)
    for s in slots:
        D(plate, s)
    add_bodies(car, [('Carrier', plate)])
    log.append('carrier ok')

    # ============================================================ REFERENCE
    ref = new_comp('Reference (not printed)')
    mx0, mx1, my0, my1 = MOD
    mz = MOD_PCB_Z + 1.6
    cx0, cx1, cy0, cy1 = CTL
    r = [
        ('Relay module PCB', tbox(mx0, mx1, my0, my1, MOD_PCB_Z, mz)),
        # as bought: 12-way output terminals along the field-wire edge, 6-way input terminal (DC+ DC- IN1-4)
        # and the S1-S4 H/L jumpers along the far edge
        ('Input terminal (DC+ DC- IN1-4)', tbox(mx0 + 1.0, mx0 + 31.5, my0 + 41.5, my0 + 49.5, mz, mz + 10.0)),
        ('H/L jumpers S1-S4', tbox(mx1 - 31.0, mx1 - 20.0, my0 + 40.0, my0 + 48.0, mz, mz + 8.5)),
        ('Controller perfboard', tbox(cx0, cx1, cy0, cy1, CTL_TOP - 1.6, CTL_TOP)),
        ('XIAO ESP32-C6', tbox(-60.0, -39.0, XIAO_CY - 8.75, XIAO_CY + 8.75, CTL_TOP + 8.5, CTL_TOP + 9.5)),
        ('XIAO shield + USB', tbox(-61.5, -46.0, XIAO_CY - 4.75, XIAO_CY + 4.75, CTL_TOP + 9.5, CTL_TOP + 12.7)),
        # 0.6" DIP rows on grid rows 0 and 6, cols 0-6: D0-D6 on row 0, 5V GND 3V3 D10 D9 D8 D7 on row 6
        ('XIAO header row D0-D6', tbox(PB(0, 0)[0] - 1.27, PB(6, 0)[0] + 1.27, PB(0, 0)[1] - 1.27, PB(0, 0)[1] + 1.27,
                                        CTL_TOP, CTL_TOP + 8.5)),
        ('XIAO header row 5V-D7', tbox(PB(0, 6)[0] - 1.27, PB(6, 6)[0] + 1.27, PB(0, 6)[1] - 1.27, PB(0, 6)[1] + 1.27,
                                        CTL_TOP, CTL_TOP + 8.5)),
        ('XL7015 board', tbox(XL_C[0] - XL_L / 2, XL_C[0] + XL_L / 2, XL_C[1] - XL_W / 2, XL_C[1] + XL_W / 2,
                              PLATE + 1.5, PLATE + 1.5 + 1.6)),
        ('XL7015 parts (caps, trimpot)', tbox(XL_C[0] - XL_L / 2 + 1, XL_C[0] + XL_L / 2 - 1, XL_C[1] - XL_W / 2,
                                              XL_C[1] + XL_W / 2, PLATE + 3.1, PLATE + 1.5 + XL_H)),
        ('470uF 63V (lying along x, leads col 20, over the board edge)',
         tcyl((CAP_AX[0], CAP_AX[2], CTL_TOP + 5.0), (CAP_AX[1], CAP_AX[2], CTL_TOP + 5.0), 5.0)),
        ('JST-PH 6p pair (D-pad), lying flat', tbox(22.0, 36.0, 5.0, 17.0, 4.6, 9.1)),
        ('JST-PH 4p pair (OLED), lying flat', tbox(38.0, 48.0, 5.0, 17.0, 4.6, 9.1)),
        # 5.08 R-C terminal, pins in cols 21/23 of row 8, wire entries facing the wall window (+y)
        ('R-C terminal', tbox(PB(21, 0)[0] - 2.54, PB(23, 0)[0] + 2.54, PB(0, 8)[1] - 4.0, PB(0, 8)[1] + 4.0,
                              CTL_TOP, CTL_TOP + 12.0)),
        # 5x20 PCB fuse holders, measured 26.8 x 10 x 17 with the cap on (pins 23 apart = 9 holes)
        ('Fuse F1 PCB holder', tbox(FUSE_X[0], FUSE_X[1], FUSE_Y[0] - 5.0, FUSE_Y[0] + 5.0, CTL_TOP, CTL_TOP + FUSE_H)),
        ('Fuse F2 PCB holder', tbox(FUSE_X[0], FUSE_X[1], FUSE_Y[1] - 5.0, FUSE_Y[1] + 5.0, CTL_TOP, CTL_TOP + FUSE_H)),
        # ---- small parts (docs/HARDWARE.md, Controller board). Signal jumpers run flat on the underside (26 AWG).
        # ---- small parts (docs/HARDWARE.md, Controller board). Wires to the XIAO solder to its header tails underneath.
        ('XIAO antenna keepout (keep empty)', tbox(-39.0, FUSE_X[0], CTL[2], CTL[3], CTL_TOP, CTL_TOP + 0.2)),
        ('100nF XIAO 5V (cols 0-1, row 7)', tbox(PB(0, 7)[0] - 1.23, PB(1, 7)[0] + 1.23, PB(0, 7)[1] - 1.25, PB(0, 7)[1] + 1.25,
                                                 CTL_TOP, CTL_TOP + 7.0)),
        ('100nF XIAO 3V3 (cols 2-3, row 7)', tbox(PB(2, 7)[0] - 1.23, PB(3, 7)[0] + 1.23, PB(0, 7)[1] - 1.25, PB(0, 7)[1] + 1.25,
                                                  CTL_TOP, CTL_TOP + 7.0)),
        ('Opt. I2C pull-up SDA 10k (row 8, cols 0-3)', tcyl((PB(0, 8)[0] + 1.3, PB(0, 8)[1], CTL_TOP + 1.1),
                                                            (PB(3, 8)[0] - 1.3, PB(0, 8)[1], CTL_TOP + 1.1), 1.1)),
        ('Opt. I2C pull-up SCL 10k (row 8, cols 3-6)', tcyl((PB(3, 8)[0] + 1.3, PB(0, 8)[1], CTL_TOP + 1.1),
                                                            (PB(6, 8)[0] - 1.3, PB(0, 8)[1], CTL_TOP + 1.1), 1.1)),
        ('1N5819 (row 9, cols 0-4)', tcyl((PB(0, 9)[0] + 2.5, PB(0, 9)[1], CTL_TOP + 1.4), (PB(4, 9)[0] - 2.5, PB(0, 9)[1], CTL_TOP + 1.4), 1.4)),
        ('1N4007 (row 8, cols 11-14)', tcyl((PB(11, 8)[0] + 1.2, PB(0, 8)[1], CTL_TOP + 1.4), (PB(14, 8)[0] - 1.2, PB(0, 8)[1], CTL_TOP + 1.4), 1.4)),
        ('1.5KE51A TVS (row 9, cols 13-19)', tcyl((PB(16, 9)[0] - 4.75, PB(0, 9)[1], CTL_TOP + 2.75), (PB(16, 9)[0] + 4.75, PB(0, 9)[1], CTL_TOP + 2.75), 2.75)),
        ('F2 -> COM wire hole (col 10, row 9)', tcyl((PB(10, 9)[0], PB(10, 9)[1], CTL_TOP), (PB(10, 9)[0], PB(10, 9)[1], CTL_TOP + 0.6), 0.6)),
        ('XL7015 cable holes (col 23, rows 4-5)', tbox(PB(23, 4)[0] - 1.0, PB(23, 4)[0] + 1.0, PB(0, 4)[1] - 1.0, PB(0, 5)[1] + 1.0, CTL_TOP, CTL_TOP + 0.6)),
        ('SHT40 module', tbox(SHT_X0, SHT_X0 + SHT_L, SHT_Y1 - SHT_W, SHT_Y1, SHT_Z, SHT_Z + SHT_T + 1.0)),
        ('#8 screw head L (slot travel)', U(tcyl((-MNT_X - MNT_HALF, MNT_Y, PLATE + MNT_BOSS), (-MNT_X - MNT_HALF, MNT_Y, PLATE + MNT_BOSS + 3), MNT_RH),
                                          U(tbox(-MNT_X - MNT_HALF, -MNT_X + MNT_HALF, MNT_Y - MNT_RH, MNT_Y + MNT_RH, PLATE + MNT_BOSS, PLATE + MNT_BOSS + 3),
                                            tcyl((-MNT_X + MNT_HALF, MNT_Y, PLATE + MNT_BOSS), (-MNT_X + MNT_HALF, MNT_Y, PLATE + MNT_BOSS + 3), MNT_RH)))),
        ('#8 screw head R (slot travel)', U(tcyl((MNT_X, MNT_Y - MNT_VHALF, PLATE + MNT_BOSS), (MNT_X, MNT_Y - MNT_VHALF, PLATE + MNT_BOSS + 3), MNT_RH),
                                          U(tbox(MNT_X - MNT_RH, MNT_X + MNT_RH, MNT_Y - MNT_VHALF, MNT_Y + MNT_VHALF, PLATE + MNT_BOSS, PLATE + MNT_BOSS + 3),
                                            tcyl((MNT_X, MNT_Y + MNT_VHALF, PLATE + MNT_BOSS), (MNT_X, MNT_Y + MNT_VHALF, PLATE + MNT_BOSS + 3), MNT_RH)))),
        ('OLED PCB', tbox(UX - OLED_W / 2, UX + OLED_W / 2, OLED_YB, OLED_YB + OLED_H, ZF - 2.4, ZF - 1.4)),
        ('OLED glass', tbox(UX - 13.35, UX + 13.35, OLED_YB + 4.6, OLED_YB + 24.0, ZF - 1.4, ZF)),
        ('OLED back parts', tbox(UX - 12.0, UX + 12.0, OLED_YB + 1, OLED_YB + 26, ZF - 3.4, ZF - 2.4)),
    ]
    for nm, c in (('10k pull-down IN4 (D10)', 0), ('10k pull-down IN1 (D1)', 1), ('10k pull-down IN2 (D2)', 2),
                  ('10k pull-down IN3 (D3)', 3), ('1k series D6 (RIGHT key)', 4)):   # under the XIAO, leads on rows 1 and 5
        x = PB(c, 0)[0]
        r.append((nm, tcyl((x, PB(0, 1)[1] + 0.6, CTL_TOP + 1.25), (x, PB(0, 5)[1] - 0.6, CTL_TOP + 1.25), 1.2)))   # 1/4 W ~2.4
    for i, dx in enumerate((-25.5, -8.5, 8.5, 25.5)):
        x = (mx0 + mx1) / 2 + dx
        r.append(('Relay K%d' % (i + 1), tbox(x - 7.75, x + 7.75, my0 + 10, my0 + 29, mz, mz + 15.5)))
        r.append(('Terminal K%d' % (i + 1), tbox(x - 7.6, x + 7.6, my0 + 0.5, my0 + 8.5, mz, mz + 10.0)))
    for name in SWITCHES:
        r.append(('SW %s body' % name, sw_box(name, -1.4, 11.4, SW_W, SW_Z0, SW_Z1)))
        if name == 'center':
            for j, (gx, gy) in enumerate(ck_guides()):
                r.append(('Centre guide pin %d (1.75 filament, %.1f long)' % (j + 1, CK_PIN_CUT),
                          tcyl((gx, gy, CK_HOLE_TOP - CK_PIN_CUT), (gx, gy, CK_HOLE_TOP - 0.05), 0.875)))
        lev = None
        s0 = LEV_PIVOT - LEV_L
        nseg = 26
        for k in range(nseg):                      # stepped approximation of the inclined lever
            sa = s0 + LEV_L * k / nseg
            sb = s0 + LEV_L * (k + 1) / nseg
            ztop = SW_Z1 + LEV_LIFT * (LEV_PIVOT - sa) / LEV_L
            seg = sw_box(name, sa, sb, LEV_W, max(SW_Z1, ztop - 1.0), ztop)
            lev = seg if lev is None else U(lev, seg)
        r.append(('SW %s lever' % name, lev))
        r.append(('SW %s pins' % name, sw_box(name, -0.4, 10.4, 0.5, SW_Z0 - 3.5, SW_Z0)))
        (ax, ay), (ux, uy) = SWITCHES[name]
        ps, pside = PINS[name]
        hx, hy, hz = DC[0] + ax + ux * ps, DC[1] + ay + uy * ps, SW_Z0 + HOLE_Z
        half = SW_W / 2 + 0.15 + 1.2
        r.append(('Pin %s (1.75 filament)' % name, tcyl((hx + uy * half, hy - ux * half, hz),
                                                        (hx - uy * half, hy + ux * half, hz), PIN_D / 2)))
    add_bodies(ref, r)
    for b in ref.bRepBodies:
        b.opacity = 0.55
    log.append('reference ok')
    build_wiring()

    try:
        lib = app.materialLibraries.itemByName('Fusion Appearance Library')
        def appear(n):
            a = des.appearances.itemByName(n)
            return a if a else des.appearances.addByCopy(lib.appearances.itemByName(n), n)
        white, grey = appear('Plastic - Matte (White)'), appear('Plastic - Matte (Gray)')
        black = appear('Plastic - Matte (Black)')
        for comp, ap in ((cov, white), (bp, white), (keys, grey), (car, grey), (grm, black)):
            for b in comp.bRepBodies:
                b.appearance = ap
    except Exception:
        log.append('appearance skipped')
    for comp in (cov, bp, keys, car, grm):
        for b in comp.bRepBodies:
            log.append('%s/%s %.1fcm3 solid=%s' % (comp.name, b.name, b.volume, b.isSolid))

try:
    build()
except Exception:
    log.append(traceback.format_exc())
print('\n'.join(log))

def run(context):
    pass
