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


namespace py = pybind11;

py::bytes serialize_shape(const TopoDS_Shape &shape);
TopoDS_Shape deserialize_shape(const py::bytes &buf);
py::bytes serialize_location(const TopLoc_Location &location);
TopLoc_Location deserialize_location(const py::bytes &buf);
std::string _test();
std::string _testOCCT();

