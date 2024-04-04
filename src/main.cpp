#include <pybind11/pybind11.h>
#include "tessellator/tessellator.h"
#include "serializer/serializer.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

PYBIND11_MODULE(ocp_addons, m) {
    
    py::class_<MeshData>(m, "MeshData")
        .def_readonly("vertices", &MeshData::vertices)
        .def_readonly("normals", &MeshData::normals)
        .def_readonly("triangles", &MeshData::triangles)
        .def_readonly("face_types", &MeshData::face_types)
        .def_readonly("triangles_per_face", &MeshData::triangles_per_face)
        .def_readonly("segments", &MeshData::segments)
        .def_readonly("segments_per_edge", &MeshData::segments_per_edge)
        .def_readonly("edge_types", &MeshData::edge_types)
        .def_readonly("obj_vertices", &MeshData::obj_vertices);

    m.doc() = R"pbdoc(
        OCP addons
        ----------

        .. currentmodule:: ocp_addons

        .. autosummary::
           :toctree: _generate

           add
           subtract
    )pbdoc";

    m.def("tessellate", &tessellate, R"pbdoc(
        Tessellate a shape

        Tessellate OCP object with a native function via pybind11 and arrow
    )pbdoc");

    m.def("serialize_shape", &serialize_shape);
    m.def("deserialize_shape", &deserialize_shape);
    m.def("serialize_location", &serialize_location);
    m.def("deserialize_location", &deserialize_location);

    m.def("_test", &_test);
    m.def("_testOCCT", &_testOCCT);

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif

}
