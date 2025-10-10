#include <TopoDS_Shape.hxx>
#include <BinTools.hxx>
#include <tessellator.h>
#include <iostream>
#include <string>
#include <pybind11/embed.h>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <TopAbs_ShapeEnum.hxx>

std::vector<TopoDS_Face> get_faces(const TopoDS_Shape &shape)
{
    TopTools_IndexedMapOfShape face_map;
    TopExp::MapShapes(shape, TopAbs_FACE, face_map);

    std::vector<TopoDS_Face> faces;
    const Standard_Integer nb_faces = face_map.Extent();
    for (Standard_Integer i = 1; i <= nb_faces; ++i)
    {
        faces.push_back(TopoDS::Face(face_map(i)));
    }
    return faces;
}

int main()
{
    pybind11::scoped_interpreter guard{};
    pybind11::gil_scoped_acquire acquire;
    pybind11::object OCP = pybind11::module_::import("OCP");
    // pybind11::object addons = pybind11::module_::import("addons");

    TopoDS_Shape shape;

    std::ifstream file("/tmp/logo.brep", std::ios::binary);
    BinTools::Read(shape, file);

    // std::vector<TopoDS_Face> faces = get_faces(shape);

    pybind11::object py_shape = pybind11::cast(shape); // If bindings support TopoDS_Shape

    MeshData mesh = tessellate(
        // faces[0], // shape
        py_shape, // shape
        0.01,     // deflection
        0.3,      // angular_tolerance
        true,     // compute_faces
        true,     // compute_edges
        true,     // parallel
        0,        // debug
        true      // timer
    );

    return 0;
}