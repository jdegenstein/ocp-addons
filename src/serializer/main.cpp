#include <pybind11/pybind11.h>
#include <pybind11/detail/internals.h>
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

// Helper function to extract TopoDS_Shape pointer from any pybind11 object
// This works around the Windows type_info mismatch issue
static TopoDS_Shape* extract_shape_ptr(py::handle obj) {
    auto *inst = reinterpret_cast<py::detail::instance *>(obj.ptr());
    void *ptr = inst->simple_value_holder[0];
    if (ptr) {
        return reinterpret_cast<TopoDS_Shape*>(ptr);
    }
    throw py::type_error("Could not extract TopoDS_Shape pointer from object");
}

// Helper function to extract TopLoc_Location pointer from any pybind11 object
static TopLoc_Location* extract_location_ptr(py::handle obj) {
    auto *inst = reinterpret_cast<py::detail::instance *>(obj.ptr());
    void *ptr = inst->simple_value_holder[0];
    if (ptr) {
        return reinterpret_cast<TopLoc_Location*>(ptr);
    }
    throw py::type_error("Could not extract TopLoc_Location pointer from object");
}

// Create a Python object wrapping a TopoDS_Shape using OCP's type system
static py::object create_ocp_shape(const TopoDS_Shape &shape) {
    py::object ocp_topoDS = py::module_::import("OCP.TopoDS");
    py::object shape_class = ocp_topoDS.attr("TopoDS_Shape");
    py::object new_obj = shape_class();  // Create default-constructed shape

    // Copy our shape data into the OCP object using same access pattern
    auto *inst = reinterpret_cast<py::detail::instance *>(new_obj.ptr());
    void *ptr = inst->simple_value_holder[0];
    if (ptr) {
        *reinterpret_cast<TopoDS_Shape*>(ptr) = shape;
        return new_obj;
    }
    throw py::type_error("Could not access TopoDS_Shape storage in OCP object");
}

// Create a Python object wrapping a TopLoc_Location using OCP's type system
static py::object create_ocp_location(const TopLoc_Location &location) {
    py::object ocp_topLoc = py::module_::import("OCP.TopLoc");
    py::object loc_class = ocp_topLoc.attr("TopLoc_Location");
    py::object new_obj = loc_class();  // Create default-constructed location

    auto *inst = reinterpret_cast<py::detail::instance *>(new_obj.ptr());
    void *ptr = inst->simple_value_holder[0];
    if (ptr) {
        *reinterpret_cast<TopLoc_Location*>(ptr) = location;
        return new_obj;
    }
    throw py::type_error("Could not access TopLoc_Location storage in OCP object");
}

py::bytes serialize_shape(py::object obj, bool tangents, bool normals) {
    TopoDS_Shape *shape_ptr = extract_shape_ptr(obj);
    std::ostringstream buf;
    BinTools::Write(*shape_ptr, buf, tangents, normals, BinTools_FormatVersion_CURRENT);
    return py::bytes(std::move(buf.str()));
}

py::object deserialize_shape(const py::bytes &buf) {
    std::istringstream stream(buf);
    TopoDS_Shape shape;
    BinTools::Read(shape, stream);
    return create_ocp_shape(shape);
}

py::bytes serialize_location(py::object obj) {
    TopLoc_Location *location_ptr = extract_location_ptr(obj);
    std::ostringstream buf;
    BinTools_OStream occtStream(buf);
    BinTools_ShapeWriter().WriteLocation(occtStream, *location_ptr);
    return py::bytes(std::move(buf.str()));
}

py::object deserialize_location(const py::bytes &buf) {
    std::istringstream stream(buf);
    BinTools_IStream occtStream(stream);
    TopLoc_Location location = *BinTools_ShapeReader().ReadLocation(occtStream);
    return create_ocp_location(location);
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
