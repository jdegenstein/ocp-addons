import json
import io
import os
import sys
import tempfile
from time import time
from pathlib import Path

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
    m = tessellate(
        obj,
        deflection,
        angular_tolerance,
        compute_faces=True,
        compute_edges=True,
        parallel=parallel,
        debug=2,
        timeit=True,
    )

    return {
        "vertices": m.vertices,
        "normals": m.normals,
        "triangles": m.triangles,
        "face_types": m.face_types,
        "edges": m.segments,
        "edge_types": m.edge_types,
        "obj_vertices": m.obj_vertices,
    }


print("test serializer:", serializer._test())
print("test serializer OCCT shape:", serializer._testOCCT())

for test_case in [0, 1, 2, 3]:

    show_results = True
    
    if test_case == 0:
        print("1 Simple box deserialized")
        file = Path("examples") / "b123.brep"
        acc = 0.002
        show_results = True

        with open(file, "rb") as f:
            obj = deserialize(f.read())
    elif test_case == 1:
        print("2 Large RC object")
        file = "examples/rc.brep"
        acc = 0.19
        show_results = False

        with open(file, "rb") as f:
            obj = deserialize(f.read())
    elif test_case == 2:
        print("3 Simple box built locally")
        if CQ:
            wp = cq.Workplane()
            box = wp.box(1, 2, 3).val()
            obj, acc = box.wrapped, 0.002
        elif BD:
            box = bd.Box(1, 2, 3)
            obj = box.wrapped
            acc = 0.002
    elif test_case == 3:
        print("4 Box imported from STL")
        if BD:
            box = bd.fillet(box.edges(), 0.3)
            bd.export_stl(box, "box.stl")
            box2 = bd.import_stl("box.stl")
            obj = box2.wrapped
            acc = 0.002
        elif CQ:
            exit(1)


    tt = time()

    mesh = tess(obj, acc, 0.3, parallel=True)


    if show_results:
        print("vertices:", mesh["vertices"])
        print("normals:", mesh["normals"])
        print("triangles:", mesh["triangles"])
        print("face_types:", mesh["face_types"])
        print("edge_types:", mesh["edge_types"])
        print("edges:", mesh["edges"])
        print("obj_vertices:", mesh["obj_vertices"])

    print("overall:", int(1000 * (time() - tt)), "ms")



