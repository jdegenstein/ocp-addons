#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <cmath>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <BinTools.hxx>
#include <IMeshTools_Parameters.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <BRepGProp_Face.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>

namespace py = pybind11;

struct FaceData {
    Standard_Real *vertices;
    Standard_Real *normals;
    Standard_Integer *triangles;
    Standard_Integer num_vertices;
    Standard_Integer num_triangles;
    Standard_Integer face_type;
};

struct EdgeData {
    Standard_Real *segments;
    Standard_Real num_segments;
    Standard_Integer edge_type;
};

struct MeshData {
    py::array_t<float> vertices;
    py::array_t<float> normals;
    py::array_t<int> triangles;
    py::array_t<int> triangles_per_face;
    py::array_t<int> face_types;
    py::array_t<float> segments;
    py::array_t<int> segments_per_edge;
    py::array_t<int> edge_types;
    py::array_t<float> obj_vertices;
};