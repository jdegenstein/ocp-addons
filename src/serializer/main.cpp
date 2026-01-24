#include <pybind11/pybind11.h>
#include <sstream>
#include <vector>

#include <TopoDS_Shape.hxx>
#include <TopLoc_Location.hxx>

#include <BinTools.hxx>
#include <BinTools_OStream.hxx>
#include <BinTools_IStream.hxx>
#include <BinTools_ShapeReader.hxx>
#include <BinTools_ShapeWriter.hxx>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

py::bytes serialize_shape(const TopoDS_Shape &shape, bool tangents, bool normals) {
    std::ostringstream buf;
    BinTools::Write(shape, buf, tangents, normals, BinTools_FormatVersion_CURRENT);
    return py::bytes(std::move(buf.str()));
}

TopoDS_Shape deserialize_shape(const py::bytes &buf) {
    std::istringstream stream(buf);
    TopoDS_Shape shape;
    BinTools::Read(shape, stream);
    return shape;
}

py::bytes serialize_location(const TopLoc_Location &location) {
    std::ostringstream buf;
    BinTools_OStream occtStream(buf);
    BinTools_ShapeWriter().WriteLocation(occtStream, location);
    return py::bytes(std::move(buf.str()));
}

TopLoc_Location deserialize_location(const py::bytes &buf) {
    std::istringstream stream(buf);
    BinTools_IStream occtStream(stream);
    // This is not a memory leak and can only be copied due to weird occt impl
    return *BinTools_ShapeReader().ReadLocation(occtStream);
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
