#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

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
