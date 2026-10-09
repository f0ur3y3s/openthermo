"""
openthermo D-pad as an ENCLOSED stand-alone module with a 6-pin 2.54 mm Dupont socket in the bottom side wall.

Run with a design document active whose name contains 'dpad module'. Builds the case from fusion_case_v3.py
(same folder) so the D-pad parts are exactly the case's, keeps only the D-pad, encloses it, and moves everything
so the D-pad centre is at the origin (Z = 0 at the back of the module).
Its Dupont pinout (GND, UP, DOWN, LEFT, RIGHT, OK) is separate from the main case's JST-PH pigtails, and harmless
if mis-mated: every pin is an input or GND. Keep it away from anything that carries 5 V. The centre key's two guide
pins are lengths of 1.75 filament (5.7 mm, CA-glued into the centre key), as in the main case:

  Front shell (PETG)        - 56 x 56 rounded box, open at the back: the key face (square-pad openings +
                              elephant-foot lips), side walls, the four carrier posts (M2 pilots), four corner
                              columns with M3 pilots for the back cover, a slot in the bottom wall for the Dupont
                              plug and a pin-1 dot beside it, and a hold-down tongue on that wall's inner face
                              (45 deg gusset) over the plugs and the header body. Print face-down, no supports.
  Back cover (PETG)         - 2 mm plate with a locating lip, four M3 clearance holes, and a holder for a 1 x 6
                              straight male pin header lying flat, pins pointing out through the wall slot, tips
                              0.5 inside the outer face. The header body drops into a pocket from above (two
                              ledges in front, full-height stops behind its ends); the side walls are fused into
                              the lip, with 45 deg base fillets. Six single-pin female Dupont housings (2.6 x 2.6,
                              measured) plug in side by side through the slot along the channel; the shell's
                              tongue holds them and the header down. Print outside-face-down, no bridges.
  Dupont holder coupon      - three one-piece holders (wall slice + floor) side by side, 1/2/3 notches:
                              plug clearance per side 0.10 / 0.15 (= module) / 0.20, header body press fit (no hold-down).
  D-pad keys (PETG)         - flexure key plate + centre key, identical to the case. Print face-up.
  D-pad switch carrier      - identical to the case.
  Reference (not printed)   - the five lever switches, filament pins and the pin header.

Wiring (pins 1-6, pin 1 at the dot = left when facing the keys): 1 GND (common, daisy-chained to every switch COM),
2 UP, 3 DOWN, 4 LEFT, 5 RIGHT, 6 OK (each switch NO). Solder the wires to the header's short tails, drop the header
into the channel, close the cover.
Fasteners: 4 x M2 x 6 (carrier to posts), 4 x M3 x 10 (back cover to the corner columns).
"""
import adsk.core, adsk.fusion, math, os, traceback

_app = adsk.core.Application.get()
if 'dpad module' not in _app.activeDocument.name:
    raise Exception('active document is not the D-pad module document: %s' % _app.activeDocument.name)
_d = adsk.fusion.Design.cast(_app.activeProduct)
_MOD = ('Cover', 'Backplate', 'Gasket (TPU)', 'D-pad keys (PETG)', 'D-pad switch carrier (PETG)',
        'Reference (not printed)', 'Bezel (PETG)', 'Front shell (PETG)', 'Back cover (PETG)',
        'Dupont holder coupon (PETG)')
_other = [o.component.name for o in _d.rootComponent.occurrences if o.component.name not in _MOD]
if _other or _d.rootComponent.bRepBodies.count:
    raise Exception('module document holds other work: %s' % _other)
for _i in range(_d.rootComponent.occurrences.count - 1, -1, -1):
    _d.rootComponent.occurrences.item(_i).deleteMe()

_here = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else r'E:\esp\openthermo\cad'
exec(open(os.path.join(_here, 'fusion_case_v3.py'), encoding='utf-8').read(), globals())

MOD_HALF, MOD_R, MOD_WALL = 28.0, 4.0, 2.0      # outline half-size, corner radius, side wall (flexure frame reaches 21.4)
MOD_BACK_T = 2.0                                # back cover plate
MOD_COL = (24.5, 2.8)                           # corner columns, full height (clear of the flexure frame corners)
MOD_M3_DEPTH = 8.0                              # M3 pilot depth in the columns
MOD_LIP = (0.15, 1.2, 2.0)                      # back cover locating lip: clearance to the wall, thickness, height
# 1 x 6 straight male pin header, 2.54 pitch (standard 11.5 pins: 6.0 mating / 2.5 body / 3.0 tail)
HDR_N, HDR_P = 6, 2.54
HDR_BODY = (HDR_N * HDR_P, 2.5, 2.5)            # plastic body: length (x), depth (y), height (z)
HDR_PIN_MATE, HDR_TAIL, HDR_PIN_W = 6.0, 3.0, 0.64
HDR_X = 0.0                                     # header centre along the bottom wall (rel. D-pad centre)
PLUG_CELL = 2.6                                 # measured: single-pin female Dupont housings, 2.6 x 2.6 x 12 long
PLUG_W, PLUG_H = HDR_N * PLUG_CELL, PLUG_CELL   # six housings side by side = 15.6 x 2.6
PLUG_CLR = 0.15                                 # per side round the plug(s) in the slot/channel (fit-tested)
HDR_BODY_CLR = 0.0                              # per side round the header body in its pocket (drops in from above)
DUPONT_VARIANTS = [(0.10, 0.0), (0.15, 0.0), (0.20, 0.0)]   # coupon: (plug, body) clearance per side; #2 = module
DUPONT_PITCH, DUPONT_DX = 26.0, 70.0              # coupon strip: holder pitch, offset from the module (x)
PIN_REC = 0.5                                   # pin tips this far inside the outer face
PLUG_LEDGE = (0.6, 0.6)                         # end ledges the plug seats on: width in from each end, depth (y)
HDR_RAIL_W, HDR_ROOF_T = 2.4, 1.6               # channel side walls (fused into the lip); hold-down tongue thickness
HDR_STOP = (2.4, None)                          # stops behind the header body ends: length (y), height (None = full)
HDR_GUSSET = 1.9                                # 45 deg fillets at the holder's base (half-diagonal), print as slopes
MOD_WIRE = 4.5                                  # wiring room under the switch terminals
MOD_ZB = (SW_Z0 - 3.5) - MOD_WIRE - MOD_BACK_T   # back outer face


def occ_named(name):
    for o in root.occurrences:
        if o.component.name == name:
            return o
    return None


def dupont_holder(hx, yo, zc, plug_clr=None, body_clr=None, wall_gap=0.15):
    """Header holder features. yo = outer face of the wall, zc = floor. Returns (adds, slot, pin1_dot, rw, yend).
    adds go on the floor part; slot (and the dot) are cut from the wall."""
    plug_clr = PLUG_CLR if plug_clr is None else plug_clr
    body_clr = HDR_BODY_CLR if body_clr is None else body_clr
    yf = yo + PIN_REC + HDR_PIN_MATE                               # header body front face
    yb = yf + HDR_BODY[1]                                          # header body back face (tails side)
    cw = PLUG_W / 2 + plug_clr                                     # plug channel half-width
    bw = HDR_BODY[0] / 2 + body_clr                                # body pocket half-width
    ct = zc + PLUG_H + 2 * plug_clr                                # channel top
    rw = max(cw, bw) + HDR_RAIL_W
    y0 = yo + MOD_WALL + wall_gap
    tail_edge = (HDR_N - 1) * HDR_P / 2 + HDR_PIN_W / 2 + 0.2
    sh_ = ct - zc if HDR_STOP[1] is None else HDR_STOP[1]
    ye = yb + 0.1 + HDR_STOP[0]
    adds = []
    def fillet(axis, c, a0, a1):
        # 45 deg wedge along a base edge: a square bar turned 45 deg about the edge; its lower half sits in the plate
        g = HDR_GUSSET * math.sqrt(2)
        if axis == 'y':
            bar = tbox(c - g / 2, c + g / 2, a0, a1, zc - g / 2, zc + g / 2)
            m = adsk.core.Matrix3D.create(); m.setToRotation(math.radians(45), V(0, 1, 0), P(mm(c), 0, mm(zc)))
        else:
            bar = tbox(a0, a1, c - g / 2, c + g / 2, zc - g / 2, zc + g / 2)
            m = adsk.core.Matrix3D.create(); m.setToRotation(math.radians(45), V(1, 0, 0), P(0, mm(c), mm(zc)))
        tbm.transform(bar, m)
        return bar
    for sx in (-1, 1):
        def bx(a, b, ya, yb_, za, zb_):
            xa, xb = sorted((hx + sx * a, hx + sx * b))
            return tbox(xa, xb, ya, yb_, za, zb_)
        adds.append(bx(cw, rw, y0, yf - PLUG_LEDGE[1], zc - 0.01, ct))                          # channel rails
        adds.append(bx(cw - PLUG_LEDGE[0], rw, yf - PLUG_LEDGE[1], yf, zc - 0.01, ct))          # plug seat ledges
        adds.append(bx(bw, rw, yf, yb + 0.1, zc - 0.01, ct))                                    # body pocket walls
        adds.append(bx(tail_edge, rw, yb + 0.1, ye, zc - 0.01, zc + sh_))                            # stops
        xs = hx + sx * rw
        adds.append(fillet('y', xs, y0, ye))                                                    # outer base fillets
        xa, xb = sorted((hx + sx * tail_edge, xs))
        adds.append(fillet('x', ye, xa, xb))                                                    # behind the stops
    slot = tbox(hx - cw, hx + cw, yo - 1, yo + MOD_WALL + 1, zc - 1, ct)
    p1 = hx - (HDR_N - 1) * HDR_P / 2
    dot = tcyl((p1, yo + 0.4, ct + 1.5), (p1, yo - 1, ct + 1.5), 0.5)
    return adds, slot, dot, rw, ye


def holder_tongue(hx, yo, zc, rw):
    """Hold-down over the plugs and the header body, grown from the shell's bottom wall (no bridged roof on the back
    cover: that peeled off along a layer line). Flat face 0.05 over the holder; a 45 deg gusset back to the wall, so
    it prints face-down without support and any lift goes into the wall along its layers."""
    yw = yo + MOD_WALL                                              # wall inner face
    yt = yo + PIN_REC + HDR_PIN_MATE + HDR_BODY[1]                  # over the header body's back face
    z0 = zc + PLUG_H + 2 * PLUG_CLR + 0.05
    zt = z0 + HDR_ROOF_T
    L = yt - yw
    t = tbox(hx - rw, hx + rw, yw - 0.5, yt, z0, zt + L)
    hs = tbox(hx - rw - 1, hx + rw + 1, yt - 100, yt, zt - 100, zt + 100)
    m = adsk.core.Matrix3D.create()
    m.setToRotation(math.radians(45), V(1, 0, 0), P(0, mm(yt), mm(zt)))
    tbm.transform(hs, m)
    I(t, hs)
    return t


def dupont_coupon():
    """Fit strip: three one-piece holders (wall slice + floor) with plug / body clearances from DUPONT_VARIANTS."""
    zb, zc = MOD_ZB, MOD_ZB + MOD_BACK_T
    yo = DC[1] - MOD_HALF
    parts = []
    for i, (pc, bcl) in enumerate(DUPONT_VARIANTS):
        hx = DC[0] + i * DUPONT_PITCH
        adds, slot, dot, rw, yend = dupont_holder(hx, yo, zc, pc, bcl, wall_gap=0.0)
        ct = zc + PLUG_H + 2 * pc
        x0, x1 = hx - rw - 1.5, hx + rw + 1.5
        c = tbox(x0, x1, yo, yend + 1.5, zb, zc)                                   # floor
        U(c, tbox(x0, x1, yo, yo + MOD_WALL, zc - 0.01, ct + 3.5))                  # wall slice
        for a in adds:
            U(c, a)
        D(c, slot)
        D(c, dot)
        for k in range(i + 1):                                                     # 1/2/3 notches on the back edge
            nx = hx - 3.0 + k * 1.6
            D(c, tbox(nx - 0.4, nx + 0.4, yend + 0.9, yend + 2.0, zb - 1, zc + 1))
        parts.append(('Holder %d (plug %.2f, body %.2f)' % (i + 1, pc, bcl), c))
    return parts


def rsq(h, z0, z1, r):
    return trrect(DC[0] - h, DC[0] + h, DC[1] - h, DC[1] + h, z0, z1, max(0.2, r))


def build_module():
    out = []
    keys = [(b.name, tbm.copy(b)) for b in occ_named('D-pad keys (PETG)').component.bRepBodies]
    car = [(b.name, tbm.copy(b)) for b in occ_named('D-pad switch carrier (PETG)').component.bRepBodies]
    refs = [(b.name, tbm.copy(b)) for b in occ_named('Reference (not printed)').component.bRepBodies
            if b.name.startswith('SW ') or b.name.startswith('Pin ')]
    for i in range(root.occurrences.count - 1, -1, -1):
        root.occurrences.item(i).deleteMe()
    zb, zc = MOD_ZB, MOD_ZB + MOD_BACK_T                  # back outer face, cover inner face / shell split plane
    cols = [(DC[0] + sx * MOD_COL[0], DC[1] + sy * MOD_COL[0]) for sx in (-1, 1) for sy in (-1, 1)]
    # ---- front shell
    sh = rsq(MOD_HALF, zc, DEPTH, MOD_R)
    D(sh, rsq(MOD_HALF - MOD_WALL, zc - 1, ZF, MOD_R - MOD_WALL))
    for cx, cy in cols:
        U(sh, tcyl((cx, cy, zc), (cx, cy, ZF + 0.01), MOD_COL[1]))
    for px, py in POSTS:
        U(sh, tcyl((px, py, CAR_Z1), (px, py, ZF + 0.01), 2.5))
    for nm in ('center',) + tuple(ANG):
        D(sh, sq_shape(nm, SQ_HOLE_CLR, ZF - 1, DEPTH + 1))
        D(sh, sq_shape(nm, SQ_HOLE_CLR + 0.4, DEPTH - 0.4, DEPTH + 1))          # elephant-foot relief
    for px, py in POSTS:
        D(sh, tcyl((px, py, CAR_Z1 - 0.1), (px, py, CAR_Z1 + 6), M2_PILOT_R))     # M2 x 6, carrier
        D(sh, tcone((px, py, CAR_Z1 - 0.01), M2_PILOT_R + 0.3, (px, py, CAR_Z1 + 0.3), M2_PILOT_R))   # lead-in: starts the screw straight
    for cx, cy in cols:
        D(sh, tcyl((cx, cy, zc - 0.1), (cx, cy, zc + MOD_M3_DEPTH), M3_PILOT_R))   # M3 x 10, back cover
    # ---- back cover
    bc = rsq(MOD_HALF, zb, zc, MOD_R)
    lc, lt, lh = MOD_LIP
    lip = rsq(MOD_HALF - MOD_WALL - lc, zc - 0.01, zc + lh, MOD_R - MOD_WALL - lc)
    D(lip, rsq(MOD_HALF - MOD_WALL - lc - lt, zc - 1, zc + lh + 1, MOD_R - MOD_WALL - lc - lt))
    for cx, cy in cols:
        D(lip, tcyl((cx, cy, zc - 1), (cx, cy, zc + lh + 1), MOD_COL[1] + 0.3))
    # header lies on the cover, pins pointing out through the bottom wall (-Y); it drops into its pocket from
    # above (ledges in front, stops behind), then the roof over the plug channel and the cover keep it in place
    hx = DC[0] + HDR_X
    yo = DC[1] - MOD_HALF                                          # outer face of the bottom wall
    yf = yo + PIN_REC + HDR_PIN_MATE
    yb = yf + HDR_BODY[1]
    adds, slot, dot, rw, yend = dupont_holder(hx, yo, zc)
    D(sh, slot)
    U(sh, holder_tongue(hx, yo, zc, rw))
    D(sh, dot)
    D(lip, tbox(hx - (PLUG_W / 2 + PLUG_CLR), hx + (PLUG_W / 2 + PLUG_CLR), yo - 1, yend + 1, zc - 1, zc + lh + 1))
    # (the lip is cut only for the plug channel, so the holder's side walls fuse into it)
    U(bc, lip)
    for a_ in adds:
        U(bc, a_)
    for cx, cy in cols:
        D(bc, tcyl((cx, cy, zb - 1), (cx, cy, zc + 1), M3_CLEAR_R))
    # ---- reference header (body + pins)
    hzc = zc + HDR_BODY[2] / 2                                     # header body rests on the floor
    hdr = tbox(hx - HDR_BODY[0] / 2, hx + HDR_BODY[0] / 2, yf, yb, hzc - HDR_BODY[2] / 2, hzc + HDR_BODY[2] / 2)
    for i in range(HDR_N):
        px = hx + (i - (HDR_N - 1) / 2) * HDR_P
        U(hdr, tbox(px - HDR_PIN_W / 2, px + HDR_PIN_W / 2, yf - HDR_PIN_MATE, yb + HDR_TAIL,
                    hzc - HDR_PIN_W / 2, hzc + HDR_PIN_W / 2))
    refs.append(('Pin header 1x6', hdr))
    # ---- checks, in place
    def vol(a, b):
        t = tbm.copy(a)
        tbm.booleanOperation(t, tbm.copy(b), BT.IntersectionBooleanType)
        return t.volume * 1000 if t.lumps.count else 0.0
    parts = [('front shell', sh), ('back cover', bc)] + [(n, b) for n, b in keys] + [('carrier', b) for _, b in car]
    for i, (na, a) in enumerate(parts):
        for nb, b in parts[i + 1:]:
            v = vol(a, b)
            if v > 0.001:
                out.append('OVERLAP %s x %s %.3f mm3' % (na, nb, v))
        v = sum(vol(a, r) for rn, r in refs)
        if na != 'carrier' and v > 0.001:
            out.append('OVERLAP %s x reference %.3f mm3' % (na, v))
    out.append('header x carrier %.3f, x shell %.3f, x switches %.3f mm3' % (
        sum(vol(hdr, b) for _, b in car), vol(hdr, sh), sum(vol(hdr, r) for rn, r in refs if rn != 'Pin header 1x6')))
    out.append('module %.1f x %.1f x %.1f mm (back at z %.2f)' % (2 * MOD_HALF, 2 * MOD_HALF, DEPTH + KEY_PROUD - zb, zb))
    # ---- move: D-pad centre to the origin, back face to z = 0
    mv = adsk.core.Matrix3D.create()
    mv.translation = V(-mm(DC[0]), -mm(DC[1]), -mm(zb))
    coupon = dupont_coupon()
    for b in [sh, bc] + [t for _, t in keys + car + refs + coupon]:
        tbm.transform(b, mv)
    mvc = adsk.core.Matrix3D.create()
    mvc.translation = V(mm(DUPONT_DX), 0, 0)
    for _, t in coupon:
        tbm.transform(t, mvc)
    comps = []
    for cname, named in (('Front shell (PETG)', [('Front shell', sh)]), ('Back cover (PETG)', [('Back cover', bc)]),
                         ('D-pad keys (PETG)', keys), ('D-pad switch carrier (PETG)', car),
                         ('Reference (not printed)', refs), ('Dupont holder coupon (PETG)', coupon)):
        c = new_comp(cname)
        add_bodies(c, named)
        for i, (n, _) in enumerate(named):                          # names do not always stick in add_bodies
            c.bRepBodies.item(i).name = n
        comps.append(c)
    try:
        white = des.appearances.itemByName('Plastic - Matte (White)')
        grey = des.appearances.itemByName('Plastic - Matte (Gray)')
        for c in comps[:2]:
            for b in c.bRepBodies:
                b.appearance = white
        for c in comps[2:4]:
            for b in c.bRepBodies:
                b.appearance = grey
        for b in comps[4].bRepBodies:
            b.opacity = 0.55
    except Exception:
        pass
    for c in comps[:4] + comps[5:]:
        for b in c.bRepBodies:
            out.append('%s/%s %.3f cm3 solid=%s lumps=%d' % (c.name, b.name, b.volume, b.isSolid, b.lumps.count))
    return out


log = []
try:
    log.extend(build_module())
except Exception:
    log.append(traceback.format_exc())
print('\n'.join(log))


def run(context):
    pass
