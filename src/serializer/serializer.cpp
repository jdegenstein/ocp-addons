#include "serializer.h"

py::bytes serialize_shape(const TopoDS_Shape &shape) {
    std::ostringstream buf;
    BinTools::Write(shape, buf);
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

