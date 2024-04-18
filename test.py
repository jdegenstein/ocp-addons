import json
import io
import os
import sys
import tempfile
from time import time

try:
    import cadquery as cq

    CQ = True
except:
    CQ = False
try:
    import build123d as bd

    BD = True
except:
    BD = False

if not (CQ or BD):
    print("Must have CadQuery or build123d to run tests")

from OCP.BinTools import BinTools
from OCP.TopoDS import TopoDS_Shape

# sys.setdlopenflags(os.RTLD_GLOBAL | os.RTLD_LAZY)

sys.path.append("build")

from ocp_addons.tessellator import tessellate
from ocp_addons import serializer


def deserialize(buffer):
    if buffer is None:
        return None

    shape = TopoDS_Shape()
    try:
        bio = io.BytesIO(buffer)
        BinTools.Read_s(shape, bio)
    except Exception:
        with tempfile.TemporaryDirectory() as tmpdirname:
            filename = os.path.join(tmpdirname, "shape.brep")
            with open(filename, "wb") as fd:
                fd.write(buffer)
            BinTools.Read_s(shape, filename)
    return shape


def decompose(array, indexes, flatten=False):
    result = []
    s = 0
    for e in indexes:
        e2 = s + e
        sub = array[s:e2]
        if flatten:
            sub = sub.reshape(-1)
        result.append(sub)
        s = e2
    return result


def tess(obj, deflection, angular_tolerance, parallel):
    m = tessellate(obj, deflection, angular_tolerance, parallel, False, True)

    t = time()
    tr = decompose(m.triangles.reshape(-1, 3), m.triangles_per_face, True)
    print("Reshape tessellation", int(1000 * (time() - t)), "ms")

    t = time()
    sg = decompose(m.segments.reshape(-1, 2, 3), m.segments_per_edge)
    print("Reshape edges", int(1000 * (time() - t)), "ms")

    return {
        "vertices": m.vertices,
        "normals": m.normals,
        "triangles": tr,
        "face_types": m.face_types,
        "edges": sg,
        "edge_types": m.edge_types,
        "obj_vertices": m.obj_vertices,
    }


brep = False

if brep:
    # file, acc, show = "examples/b123.brep", 0.002, True
    file, acc, show = "examples/rc.brep", 0.19, False

    with open(file, "rb") as f:
        obj = deserialize(f.read())
else:
    if CQ:
        obj, acc, show = cq.Workplane().box(1, 2, 3).val().wrapped, 0.002, True
    elif BD:
        obj, acc, show = bd.Box(1, 2, 3).wrapped, 0.002, True


tt = time()


mesh = tess(obj, acc, 0.3, parallel=True)


if show:
    print("vertices:", mesh["vertices"])
    print("normals:", mesh["normals"])
    print("triangles:")
    for t in mesh["triangles"]:
        print("  ", json.dumps(t.tolist()))
    print("face_types:", mesh["face_types"])
    print("edge_types:", mesh["edge_types"])
    print("edges")
    for e in mesh["edges"]:
        print("  ", json.dumps(e.tolist()))
    print("obj_vertices:", mesh["obj_vertices"])
    print("test serializer:", serializer._test())
    print("test serializer OCCT shape:", serializer._testOCCT())
    # print("test C++ string return:", serializer._testSTRreturn())
    
print("overall:", int(1000 * (time() - tt)), "ms")
