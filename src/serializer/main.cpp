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

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS_Edge.hxx>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

py::bytes serialize_shape(const TopoDS_Shape &shape) {
    std::ostringstream buf;
    std::cout << "before bintools write";
    BinTools::Write(shape, buf);
    std::cout << "after bintools write";
    std::cout << buf.str();
    std::cout << "after buf.str";
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

py::bytes _testOCCT() {
    auto pt1 = gp_Pnt(0,0,0);
    auto pt2 = gp_Pnt(1,2,3);
    auto line = BRepBuilderAPI_MakeEdge(pt1,pt2).Edge();
    std::ostringstream buf;
    std::cout << "before bintools write";
    BinTools::Write(line, buf);
    std::cout << "after bintools write";
    std::cout << buf.str();
    std::cout << "after buf.str";
    return py::bytes(std::move(buf.str()));
}

py::bytes _testSTRreturn() {
    std::string string1 = "b'\nOpen CASCADE Topology V4, (c) Open Cascade\nLocations 0\nCurve2ds 0\nCurves 1\n\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00*$`\xe5\xce\x1a\xd1?*$`\xe5\xce\x1a\xe1??6\x10X6\xa8\xe9?Polygon3D 0\nPolygonOnTriangulations 0\nSurfaces 0\nTriangulations 0\n\nTShapes 3\n\x07H\xaf\xbc\x9a\xf2\xd7z>\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x01\x01\x00\x01*\x07H\xaf\xbc\x9a\xf2\xd7z>\x00\x00\x00\x00\x00\x00\xf0?\x00\x00\x00\x00\x00\x00\x00@\x00\x00\x00\x00\x00\x00\x08@\x00\x00\x01\x00\x01\x01\x00\x01*\x06H\xaf\xbc\x9a\xf2\xd7z>\x01\x01\x00\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00I?h\x11\xea\xee\r@\x00\x01\x01\x00\x01\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x01\x02\x00\x00\x00\x00\x00\x00\x00*\x00\x01\x00\x00\x00\x00\x00\x00\x00'";
    return py::bytes(string1);
}

void register_serializer(pybind11::module_ &m_gbl) {
    auto m = m_gbl.def_submodule("serializer");

    m.def("serialize_shape", &serialize_shape);
    m.def("deserialize_shape", &deserialize_shape);
    m.def("serialize_location", &serialize_location);
    m.def("deserialize_location", &deserialize_location);

    m.def("_test", &_test);
    m.def("_testOCCT", &_testOCCT);
    m.def("_testSTRreturn", &_testSTRreturn);

    m.doc() = R"pbdoc(
        OCP Serializer 1.0.0
        -----------------------
        Serialize OCCT objects for OCP
    )pbdoc";
}
