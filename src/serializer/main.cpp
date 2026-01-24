#include <pybind11/pybind11.h>
#include <sstream>
#include <vector>

#include <BinTools.hxx>
#include <BinTools_OStream.hxx>
#include <BinTools_IStream.hxx>
#include <BinTools_ShapeReader.hxx>
#include <BinTools_ShapeWriter.hxx>

#include "../ocp_utils.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;
using namespace ocp_utils;

py::bytes serialize_shape(py::object obj, bool tangents, bool normals) {
    TopoDS_Shape *shape_ptr = extract_shape(obj);
    std::ostringstream buf;
    BinTools::Write(*shape_ptr, buf, tangents, normals, BinTools_FormatVersion_CURRENT);
    return py::bytes(std::move(buf.str()));
}

py::object deserialize_shape(const py::bytes &buf) {
    std::istringstream stream(buf);
    TopoDS_Shape shape;
    BinTools::Read(shape, stream);
    return create_shape(shape);
}

py::bytes serialize_location(py::object obj) {
    TopLoc_Location *location_ptr = extract_location(obj);
    std::ostringstream buf;
    BinTools_OStream occtStream(buf);
    BinTools_ShapeWriter().WriteLocation(occtStream, *location_ptr);
    return py::bytes(std::move(buf.str()));
}

py::object deserialize_location(const py::bytes &buf) {
    std::istringstream stream(buf);
    BinTools_IStream occtStream(stream);
    TopLoc_Location location = *BinTools_ShapeReader().ReadLocation(occtStream);
    return create_location(location);
}

std::string _test() {
    return "Ok";
}

std::string _testOCCT() {
    TopoDS_Shape shape;
    return "Ok";
}

void register_serializer(pybind11::module_ &m_gbl) {
    auto m = m_gbl.def_submodule("serializer");

    m.def("serialize_shape", &serialize_shape, py::arg("shape"), py::arg("tangents") = false, py::arg("normals") = false,
        R"pbdoc(
            Serialize a TopoDS_Shape to bytes.

            Args:
                shape: The TopoDS_Shape to serialize.
                tangents: Whether to include tangent data in the serialization.
                normals: Whether to include normal data in the serialization.

            Returns:
                bytes: The serialized shape data.
        )pbdoc");
    m.def("deserialize_shape", &deserialize_shape, py::arg("buf"),
        R"pbdoc(
            Deserialize bytes to a TopoDS_Shape.

            Args:
                buf: The bytes to deserialize.

            Returns:
                TopoDS_Shape: The deserialized shape.
        )pbdoc");
    m.def("serialize_location", &serialize_location);
    m.def("deserialize_location", &deserialize_location);

    m.def("_test", &_test);
    m.def("_testOCCT", &_testOCCT);

    m.doc() = R"pbdoc(
        OCP Serializer 1.0.0
        -----------------------
        Serialize OCCT objects for OCP
    )pbdoc";
}
