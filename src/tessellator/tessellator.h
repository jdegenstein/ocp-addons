#include <BinTools.hxx>
#include <BRep_Builder.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <IMeshTools_Parameters.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

struct FaceData
{
    Standard_Real *vertices;
    Standard_Real *normals;
    Standard_Integer *triangles;
    Standard_Integer num_vertices;
    Standard_Integer num_triangles;
    Standard_Integer face_type;
};

struct EdgeData
{
    Standard_Real *segments;
    Standard_Integer num_segments;
    Standard_Integer edge_type;
};

// threejs needs 32 bit decimal numbers, hence float
struct MeshData
{
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

MeshData tessellate(TopoDS_Shape shape, double deflection, double angular_tolerance,
                    bool compute_faces, bool compute_edges, bool parallel, int debug, bool timeit);

template <typename T>
std::string readable_typename()
{
    int status = 0;
    std::unique_ptr<char, void (*)(void *)> res{
        abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status),
        std::free};
    return (status == 0 && res) ? res.get() : typeid(T).name();
}