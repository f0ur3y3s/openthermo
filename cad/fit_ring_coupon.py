"""
openthermo cover-to-backplate FIT TEST: thin perimeter rings cut from the real cover and backplate.

Run with a design document active whose name contains 'fit test'. Builds the case from fusion_case_v3.py (same
folder) and keeps a 12 mm band round the outside of each part:

  Cover fit ring (PETG)      - the cover's side walls from the open edge up to z = 12: the snap grooves, the
                               open-edge lead-in, the bottom M3 screw hole and the ends of the divider tongues.
                               Print with the cut face (z = 12) on the bed, i.e. the same way up as the real cover
                               (face-down), so the mating edge prints at the top as it will for real.
  Backplate fit ring (PETG)  - the backplate's rim and plate edge: the snap beads, the foot relief, the bottom
                               screw boss and the ends of the divider ribs with their tongue slots. Print plate-down.

Push the ring onto the backplate ring: it should slide on, click over the two beads, sit flat, and the M3 bottom
screw should line up. About 37 g of filament together, against ~130 g for the real cover and backplate.
"""
import adsk.core, adsk.fusion, math, os, traceback

_app = adsk.core.Application.get()
if 'fit test' not in _app.activeDocument.name:
    raise Exception('active document is not the fit test document: %s' % _app.activeDocument.name)
_d = adsk.fusion.Design.cast(_app.activeProduct)
_OWN = ('Cover', 'Backplate', 'Gasket (TPU)', 'D-pad keys (PETG)', 'D-pad switch carrier (PETG)',
        'Reference (not printed)', 'Cover fit ring (PETG)', 'Backplate fit ring (PETG)')
_other = [o.component.name for o in _d.rootComponent.occurrences if o.component.name not in _OWN]
if _other or _d.rootComponent.bRepBodies.count:
    raise Exception('fit test document holds other work: %s' % _other)
for _i in range(_d.rootComponent.occurrences.count - 1, -1, -1):
    _d.rootComponent.occurrences.item(_i).deleteMe()

_here = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else r'E:\esp\openthermo\cad'
exec(open(os.path.join(_here, 'fusion_case_v3.py'), encoding='utf-8').read(), globals())

BAND = 12.0                                     # band width in from the outside
COVER_RING_TOP = 12.0                           # cover ring height (covers the tongue ends at 10.4..)


def occ_named(name):
    for o in root.occurrences:
        if o.component.name == name:
            return o
    return None


def band(z0, z1):
    b = trrect(-OW / 2 - 1, OW / 2 + 1, -OH / 2 - 1, OH / 2 + 1, z0, z1, R_OUT + 1)
    D(b, trrect(-OW / 2 + BAND, OW / 2 - BAND, -OH / 2 + BAND, OH / 2 - BAND, z0 - 1, z1 + 1, max(0.5, R_OUT - BAND)))
    return b


def build_rings():
    out = []
    cov = tbm.copy(occ_named('Cover').component.bRepBodies.item(0))
    bp = tbm.copy(occ_named('Backplate').component.bRepBodies.item(0))
    for i in range(root.occurrences.count - 1, -1, -1):
        root.occurrences.item(i).deleteMe()
    I(cov, band(-1.0, COVER_RING_TOP))
    I(bp, band(-1.0, DEPTH))

    def vol(a, b):
        t = tbm.copy(a)
        tbm.booleanOperation(t, tbm.copy(b), BT.IntersectionBooleanType)
        return t.volume * 1000 if t.lumps.count else 0.0
    out.append('assembled overlap %.3f mm3' % vol(cov, bp))
    for d in (1.0, 3.0, 5.0):
        c = tbm.copy(cov)
        m = adsk.core.Matrix3D.create()
        m.translation = V(0, 0, mm(d))
        tbm.transform(c, m)
        out.append('sliding on, %.0f mm out: overlap %.2f mm3 (snap beads only)' % (d, vol(c, bp)))
    for cname, bname, b in (('Cover fit ring (PETG)', 'Cover fit ring', cov), ('Backplate fit ring (PETG)', 'Backplate fit ring', bp)):
        c = new_comp(cname)
        add_bodies(c, [(bname, b)])
        c.bRepBodies.item(0).name = bname
        out.append('%s %.2f cm3 solid=%s lumps=%d' % (bname, b.volume, b.isSolid, b.lumps.count))
    return out


log = []
try:
    log.extend(build_rings())
except Exception:
    log.append(traceback.format_exc())
print('\n'.join(log))


def run(context):
    pass
