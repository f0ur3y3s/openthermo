"""
openthermo D-pad test coupon (square pad: tolerance + clickability).
Run in an EMPTY Fusion design document. It builds the full case model from fusion_case_v3.py
(same folder), then keeps only what the D-pad test needs:

  1. Test front plate  - the cover's front face around the D-pad (square key openings, elephant-foot lips,
                         carrier screw posts), cut out as a 44 x 44 mm coupon. Print face-down.
  2. Switch carrier    - the real carrier, unchanged. Print plate-down. Fix with 4 x M2 x 6.
  3. Keys            - the real square flexure key plate (arrow keys on folded arms, no nubs) + the centre key.
                         Print face-up, no supports.
  4. Cradle strip      - three single-switch cradles side by side with different fits:
                           #1 (1 notch):  crush-rib bite 0.05, far pin hole r 0.85
                           #2 (2 notches): crush-rib bite 0.10, far pin hole r 0.90   <- current design
                           #3 (3 notches): crush-rib bite 0.15, far pin hole r 0.95
  5. Reference (not printed) - switches and filament pins, for checking.
"""
import adsk.core, adsk.fusion, math, os, traceback

# start clean: this script owns every component in the test document
_d = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct)
_TEST = ('Cover', 'Backplate', 'Gasket (TPU)', 'D-pad keys (PETG)', 'D-pad switch carrier (PETG)',
         'Reference (not printed)', 'Test front plate (PETG)', 'Cradle tolerance strip (PETG)',
         'D-pad keys TPU-capped (PETG + TPU)', 'D-pad flexure key plate (PETG)',
         'Square pad: front plate (PETG)', 'Square pad: keys (PETG)', 'Nub tuning keys (PETG)',
         'Square flexure: key plate (PETG)', 'Square flexure: carrier (PETG)')
if 'dpad test' not in adsk.core.Application.get().activeDocument.name:
    raise Exception('active document is not the D-pad test document: %s' % adsk.core.Application.get().activeDocument.name)
_other = [o.component.name for o in _d.rootComponent.occurrences if o.component.name not in _TEST]
if _other or _d.rootComponent.bRepBodies.count:
    raise Exception('test document holds other work: %s' % _other)
for _i in range(_d.rootComponent.occurrences.count - 1, -1, -1):
    _d.rootComponent.occurrences.item(_i).deleteMe()

_here = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else r'E:\esp\openthermo\cad'
exec(open(os.path.join(_here, 'fusion_case_v3.py'), encoding='utf-8').read(), globals())

COUPON = 22.0                                   # half-size of the front-plate coupon around the D-pad centre
VARIANTS = [(0.05, 0.85), (0.10, 0.90), (0.15, 0.95)]   # (crush-rib bite, far pin-hole radius)
STRIP_X0, STRIP_Y0 = 85.0, -20.0                # where the cradle strip is placed (clear of the coupon)

def occ_named(name):
    for o in root.occurrences:
        if o.component.name == name:
            return o
    return None


def cradle(x0, y0, bite, far_r):
    """One straight cradle, switch axis +Y, actuation point at (x0, y0); floor top at z = 1.2."""
    fl = 1.2
    h = fl + SW_H + 0.65
    hw = SW_W / 2 + 0.15                                   # inner half-width
    b = tbox(x0 - hw - 1.2, x0 + hw + 1.2, y0 - 2.75, y0 + 12.75, 0, fl)
    for sx in (-1, 1):                                     # side walls
        xa, xb = sorted((x0 + sx * hw, x0 + sx * (hw + 1.2)))
        U(b, tbox(xa, xb, y0 - 1.4, y0 + 12.75, fl, h))
    U(b, tbox(x0 - hw - 1.2, x0 + hw + 1.2, y0 + 11.55, y0 + 12.75, fl, h))     # end wall
    U(b, tbox(x0 - 2.0, x0 + 2.0, y0 - 2.65, y0 - 1.45, fl, fl + 1.3))         # tip stop
    rc = hw + RIB_R - (0.15 + bite)
    for sy in (2.0, 8.5):
        for sx in (-1, 1):
            U(b, tcyl((x0 + sx * rc, y0 + sy, fl), (x0 + sx * rc, y0 + sy, h), RIB_R))
    se = 11.55 + RIB_R - (0.15 + bite)
    U(b, tcyl((x0, y0 + se, fl), (x0, y0 + se, h), RIB_R))
    D(b, tbox(x0 - SLOT_W / 2, x0 + SLOT_W / 2, y0 - 1.2, y0 + 11.2, -1, fl + 1))  # terminal opening
    hz = fl + HOLE_Z
    hy = y0 + 8.25
    reach = hw + 1.2 + 0.6
    D(b, tcyl((x0, hy, hz), (x0 + reach, hy, hz), PIN_ENTRY_R))              # entry (+X side)
    D(b, tcone((x0 + hw + 0.9, hy, hz), PIN_ENTRY_R, (x0 + hw + 1.25, hy, hz), PIN_ENTRY_R + 0.35))
    D(b, tcyl((x0, hy, hz), (x0 - reach, hy, hz), far_r))                    # far wall
    return b


def run_coupon():
    log2 = []
    # ---- 1. front-plate coupon from the cover
    cov = occ_named('Cover').component.bRepBodies.item(0)
    plate = tbm.copy(cov)
    I(plate, tbox(DC[0] - COUPON, DC[0] + COUPON, DC[1] - COUPON, DC[1] + COUPON, CAR_Z1 - 0.1, DEPTH + 1))
    # ---- 5. reference copies (switch bodies, levers, pins, filament pins)
    refo = occ_named('Reference (not printed)')
    refs = [(b.name, tbm.copy(b)) for b in refo.component.bRepBodies
            if b.name.startswith('SW ') or b.name.startswith('Pin ')]
    # ---- remove what the test does not need
    for n in ('Cover', 'Backplate', 'Gasket (TPU)', 'Reference (not printed)'):
        o = occ_named(n)
        if o:
            o.deleteMe()
    fp = new_comp('Test front plate (PETG)')
    add_bodies(fp, [('Front plate coupon', plate)])
    # ---- 4. cradle strip
    st = new_comp('Cradle tolerance strip (PETG)')
    strip = None
    pitch = SW_W + 2.7 + 3.0
    for i, (bite, far_r) in enumerate(VARIANTS):
        c = cradle(STRIP_X0 + i * pitch, STRIP_Y0, bite, far_r)
        strip = c if strip is None else U(strip, c)
        for k in range(i + 1):                                             # ID notches on the front edge
            nx = STRIP_X0 + i * pitch - 2.0 + k * 1.6
            D(strip, tbox(nx - 0.4, nx + 0.4, STRIP_Y0 - 3.0, STRIP_Y0 - 2.15, -1, 2))
    # tie the three cradles together along the floor
    U(strip, tbox(STRIP_X0 - 4.5, STRIP_X0 + 2 * pitch + 4.5, STRIP_Y0 - 2.75, STRIP_Y0 + 12.75, 0, 1.2))
    for i in range(len(VARIANTS)):                                         # re-open the terminal slots
        x0 = STRIP_X0 + i * pitch
        D(strip, tbox(x0 - SLOT_W / 2, x0 + SLOT_W / 2, STRIP_Y0 - 1.2, STRIP_Y0 + 11.2, -1, 2.5))
        for k in range(i + 1):
            nx = x0 - 2.0 + k * 1.6
            D(strip, tbox(nx - 0.4, nx + 0.4, STRIP_Y0 - 3.0, STRIP_Y0 - 2.15, -1, 2))
    add_bodies(st, [('Cradle strip', strip)])
    ref = new_comp('Reference (not printed)')
    add_bodies(ref, refs)
    for b in ref.bRepBodies:
        b.opacity = 0.55
    try:
        grey = des.appearances.itemByName('Plastic - Matte (Gray)')
        white = des.appearances.itemByName('Plastic - Matte (White)')
        for b in fp.bRepBodies:
            b.appearance = white
        for b in st.bRepBodies:
            b.appearance = grey
    except Exception:
        pass
    for c in (fp, st):
        for b in c.bRepBodies:
            log2.append('%s/%s %.2f cm3 solid=%s' % (c.name, b.name, b.volume, b.isSolid))
    return log2


def check_keys():
    """Pairwise overlap of every key with the plate coupon, carrier, switches and the other keys, in place."""
    out = []
    keys = [b for b in occ_named('D-pad keys (PETG)').component.bRepBodies]
    others = [('plate', occ_named('Test front plate (PETG)')), ('carrier', occ_named('D-pad switch carrier (PETG)')),
              ('switches', occ_named('Reference (not printed)'))]
    def vol(a, b):
        t = tbm.copy(a)
        tbm.booleanOperation(t, tbm.copy(b), BT.IntersectionBooleanType)
        return t.volume * 1000 if t.lumps.count else 0.0
    for k in keys:
        parts = ['%s %.2f' % (lab, sum(vol(k, bb) for bb in o.component.bRepBodies)) for lab, o in others]
        parts.append('keys %.2f' % sum(vol(k, k2) for k2 in keys if k2 != k))
        out.append('%s x %s mm3' % (k.name, ', '.join(parts)))
    return out


try:
    log.extend(run_coupon())
    log.extend(check_keys())
except Exception:
    log.append(traceback.format_exc())
print('\n'.join(log))


def run(context):
    pass
