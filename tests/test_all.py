from pathlib import Path
from time import time

import pytest

from ocp_addons.tessellator import tessellate
from ocp_addons import serializer

try:
    import cadquery as cq

    CQ = True
except Exception:
    CQ = False

try:
    import build123d as bd

    BD = True
except Exception:
    BD = False


def almost_equal(result, expected, tolerance=1e-6):
    """
    Compare two lists/arrays of floats for approximate equality.

    Args:
        result: The actual result (list, tuple, or array-like)
        expected: The expected values (list, tuple, or array-like)
        tolerance: Maximum absolute difference allowed (default: 1e-6)

    Returns:
        bool: True if all elements are within tolerance, False otherwise

    Raises:
        AssertionError: If lengths don't match or elements differ by more than tolerance
    """

    # Check lengths match
    if len(result) != len(expected):
        raise AssertionError(
            f"Length mismatch: result has {len(result)} elements, "
            f"expected has {len(expected)} elements"
        )

    # Check each element
    for i, (actual, expect) in enumerate(zip(result, expected)):
        diff = abs(actual - expect)
        if diff > tolerance:
            raise AssertionError(
                f"Element at index {i} differs by {diff:.2e}: "
                f"result={actual}, expected={expect}, tolerance={tolerance}"
            )

    return True


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
        "triangles_per_face": m.triangles_per_face,
        "segments_per_edge": m.segments_per_edge,
    }


# Vertices assertion
expected_vertices = [
    -0.5,
    -1.0,
    -1.5,
    -0.5,
    -1.0,
    1.5,
    -0.5,
    1.0,
    -1.5,
    -0.5,
    1.0,
    1.5,
    0.5,
    -1.0,
    -1.5,
    0.5,
    -1.0,
    1.5,
    0.5,
    1.0,
    -1.5,
    0.5,
    1.0,
    1.5,
    -0.5,
    -1.0,
    -1.5,
    0.5,
    -1.0,
    -1.5,
    -0.5,
    -1.0,
    1.5,
    0.5,
    -1.0,
    1.5,
    -0.5,
    1.0,
    -1.5,
    0.5,
    1.0,
    -1.5,
    -0.5,
    1.0,
    1.5,
    0.5,
    1.0,
    1.5,
    -0.5,
    -1.0,
    -1.5,
    -0.5,
    1.0,
    -1.5,
    0.5,
    -1.0,
    -1.5,
    0.5,
    1.0,
    -1.5,
    -0.5,
    -1.0,
    1.5,
    -0.5,
    1.0,
    1.5,
    0.5,
    -1.0,
    1.5,
    0.5,
    1.0,
    1.5,
]

# Normals assertion
expected_normals = [
    -1.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
]

# Triangles assertion (integers - exact match)
expected_triangles = [
    1,
    2,
    0,
    1,
    3,
    2,
    5,
    4,
    6,
    5,
    6,
    7,
    11,
    8,
    9,
    11,
    10,
    8,
    15,
    13,
    12,
    15,
    12,
    14,
    19,
    16,
    17,
    19,
    18,
    16,
    23,
    21,
    20,
    23,
    20,
    22,
]

# Triangles per face assertion (integers - exact match)
expected_triangles_per_face = [2, 2, 2, 2, 2, 2]

# Face types assertion
expected_face_types = [0, 0, 0, 0, 0, 0]

# Edge types assertion
expected_edge_types = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

# Edges/segments assertion
expected_segments = [
    -0.5,
    -1.0,
    -1.5,
    -0.5,
    -1.0,
    1.5,
    -0.5,
    -1.0,
    1.5,
    -0.5,
    1.0,
    1.5,
    -0.5,
    1.0,
    -1.5,
    -0.5,
    1.0,
    1.5,
    -0.5,
    -1.0,
    -1.5,
    -0.5,
    1.0,
    -1.5,
    0.5,
    -1.0,
    -1.5,
    0.5,
    -1.0,
    1.5,
    0.5,
    -1.0,
    1.5,
    0.5,
    1.0,
    1.5,
    0.5,
    1.0,
    -1.5,
    0.5,
    1.0,
    1.5,
    0.5,
    -1.0,
    -1.5,
    0.5,
    1.0,
    -1.5,
    -0.5,
    -1.0,
    -1.5,
    0.5,
    -1.0,
    -1.5,
    -0.5,
    -1.0,
    1.5,
    0.5,
    -1.0,
    1.5,
    -0.5,
    1.0,
    -1.5,
    0.5,
    1.0,
    -1.5,
    -0.5,
    1.0,
    1.5,
    0.5,
    1.0,
    1.5,
]

# Object vertices assertion
expected_obj_vertices = [
    -0.5,
    -1.0,
    1.5,
    -0.5,
    -1.0,
    -1.5,
    -0.5,
    1.0,
    1.5,
    -0.5,
    1.0,
    -1.5,
    0.5,
    -1.0,
    1.5,
    0.5,
    -1.0,
    -1.5,
    0.5,
    1.0,
    1.5,
    0.5,
    1.0,
    -1.5,
]

# Segments per edge assertion (integers - exact match)
expected_segments_per_edge = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]


def test_serializer():
    """Test basic serializer functionality"""
    assert serializer._test() == "Ok"


def test_serializer_occt():
    """Test serializer with OCCT shape"""
    assert serializer._testOCCT() == "Ok"


def test_simple_box_deserialized():
    """Test tessellation of simple box from deserialized BREP file"""
    file = Path("examples") / "b123.brep"
    acc = 0.002

    with open(file, "rb") as f:
        obj = serializer.deserialize_shape(f.read())

    mesh = tess(obj, acc, 0.3, parallel=True)

    # Assertions
    assert almost_equal(mesh["vertices"], expected_vertices)
    assert almost_equal(mesh["normals"], expected_normals)
    assert almost_equal(mesh["edges"], expected_segments)
    assert almost_equal(mesh["obj_vertices"], expected_obj_vertices)
    assert list(mesh["triangles"]) == expected_triangles
    assert list(mesh["face_types"]) == expected_face_types
    assert list(mesh["edge_types"]) == expected_edge_types
    assert list(mesh["triangles_per_face"]) == expected_triangles_per_face
    assert list(mesh["segments_per_edge"]) == expected_segments_per_edge


def test_large_rc_object():
    """Test tessellation of large RC object from BREP file"""
    file = Path("examples") / "rc.brep"
    acc = 0.19

    with open(file, "rb") as f:
        obj = serializer.deserialize_shape(f.read())

    mesh = tess(obj, acc, 0.3, parallel=True)

    # Assertions
    assert len(mesh["vertices"]) == 2241075
    assert len(mesh["normals"]) == 2241075
    assert len(mesh["triangles"]) == 3059676
    assert len(mesh["face_types"]) == 10665
    assert len(mesh["edge_types"]) == 26768
    assert len(mesh["edges"]) == 1365360
    assert len(mesh["obj_vertices"]) == 49674
    assert len(mesh["triangles_per_face"]) == 10665
    assert len(mesh["segments_per_edge"]) == 26768

    assert almost_equal(mesh["vertices"][:3], [14.25, 10.609, 22.5])
    assert almost_equal(mesh["normals"][:3], [-1.0, 0.0, 0.0])
    assert almost_equal(mesh["edges"][:3], [14.25, 10.609, 22.5])
    assert list(mesh["triangles"][:3]) == [1, 43, 0]
    assert list(mesh["triangles_per_face"][:3]) == [84, 40, 94]
    assert list(mesh["face_types"][:3]) == [1, 0, 0]
    assert list(mesh["segments_per_edge"][:3]) == [42, 1, 42]
    assert list(mesh["edge_types"][:3]), [1, 6, 1]
    assert list(mesh["obj_vertices"][:3]), [14.25, 10.609, 22.5]


@pytest.mark.skipif(not (CQ or BD), reason="Requires CadQuery or build123d")
def test_simple_box_built_locally():
    """Test tessellation of locally built simple box"""
    if CQ:
        wp = cq.Workplane()
        box = wp.box(1, 2, 3).val()
        obj = box.wrapped
    elif BD:
        box = bd.Box(1, 2, 3)
        obj = box.wrapped

    acc = 0.002

    mesh = tess(obj, acc, 0.3, parallel=True)

    # Assertions
    assert almost_equal(mesh["vertices"], expected_vertices)
    assert almost_equal(mesh["normals"], expected_normals)
    assert almost_equal(mesh["obj_vertices"], expected_obj_vertices)
    assert almost_equal(mesh["edges"], expected_segments)
    assert list(mesh["triangles"]) == expected_triangles
    assert list(mesh["face_types"]) == expected_face_types
    assert list(mesh["edge_types"]) == expected_edge_types
    assert list(mesh["triangles_per_face"]) == expected_triangles_per_face
    assert list(mesh["segments_per_edge"]) == expected_segments_per_edge


@pytest.mark.skipif(not BD, reason="Requires build123d")
def test_box_imported_from_stl():
    """Test tessellation of box imported from STL file"""
    # Create and export a filleted box
    box = bd.Box(1, 2, 3)
    box = bd.fillet(box.edges(), 0.3)
    bd.export_stl(box, "box.stl")

    # Import from STL
    box2 = bd.import_stl("box.stl")
    obj = box2.wrapped
    acc = 0.002

    mesh = tess(obj, acc, 0.3, parallel=True)

    # Assertions
    assert len(mesh["vertices"]) == 14184
    assert len(mesh["normals"]) == 14184
    assert len(mesh["triangles"]) == 28356
    assert len(mesh["face_types"]) == 1
    assert len(mesh["edge_types"]) == 9452
    assert len(mesh["edges"]) == 170136
    assert len(mesh["obj_vertices"]) == 0
    assert len(mesh["triangles_per_face"]) == 1
    assert len(mesh["segments_per_edge"]) == 9452

    assert almost_equal(mesh["vertices"][:3], [-0.5, -0.7, 1.2])
    assert almost_equal(mesh["normals"][:3], [-1.0, -0.00012837978, 0.00014964596])
    assert almost_equal(mesh["edges"][:3], [-0.5, -0.7, 1.2])
    assert list(mesh["triangles"][:3]) == [0, 1, 2]
    assert list(mesh["triangles_per_face"]) == [9452]
    assert list(mesh["face_types"]) == [10]
    assert list(mesh["segments_per_edge"][:3]) == [3, 3, 3]
    assert list(mesh["edge_types"][:3]), [0, 0, 0]

    # Cleanup
    Path("box.stl").unlink(missing_ok=True)


if __name__ == "__main__":
    # Allow running as script for debugging
    pytest.main([__file__, "-v", "-s"])
