/**
 * @file tessellator.h
 * @brief Tessellation utilities for converting OpenCASCADE shapes to mesh data for rendering
 *
 * This header provides functionality to tessellate 3D CAD shapes from OpenCASCADE into
 * mesh representations suitable for visualization in web-based renderers like Three.js.
 * The tessellation process converts BREP (Boundary Representation) geometry into
 * triangulated meshes and polyline segments.
 */

#include <BinTools.hxx>
#include <BRep_Builder.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Result.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <IMeshTools_Parameters.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <ShapeFix_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
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

/**
 * @struct FaceData
 * @brief Container for tessellated face geometry data
 *
 * Holds the raw geometric data for a single tessellated face including
 * vertex coordinates, surface normals, triangle indices, and face classification.
 *
 * @var vertices Pointer to array of vertex coordinates (x,y,z triplets)
 * @var normals Pointer to array of normal vectors (nx,ny,nz triplets)
 * @var triangles Pointer to array of triangle vertex indices
 * @var num_vertices Total number of vertices in the face
 * @var num_triangles Total number of triangles in the face
 * @var face_type Classification type of the face geometry
 */
struct FaceData
{
    Standard_Real *vertices;
    Standard_Real *normals;
    Standard_Integer *triangles;
    Standard_Integer num_vertices;
    Standard_Integer num_triangles;
    Standard_Integer face_type;
};

/**
 * @struct EdgeData
 * @brief Container for tessellated edge geometry data
 *
 * Holds the geometric data for a single tessellated edge as line segments.
 *
 * @var segments Pointer to array of line segment endpoints
 * @var num_segments Total number of line segments in the edge
 * @var edge_type Classification type of the edge geometry
 */

struct EdgeData
{
    Standard_Real *segments;
    Standard_Integer num_segments;
    Standard_Integer edge_type;
};

/**
 * @struct MeshData
 * @brief Complete mesh representation for web rendering
 *
 * Contains all tessellated geometry data formatted as NumPy arrays for efficient
 * transfer to Python and subsequent use in web-based 3D rendering engines.
 * Uses 32-bit floats for compatibility with Three.js and WebGL.
 *
 * @var vertices Combined vertex coordinates for all faces
 * @var normals Combined normal vectors for all faces
 * @var triangles Combined triangle indices for all faces
 * @var triangles_per_face Number of triangles per individual face
 * @var face_types Classification types for each face
 * @var segments Combined line segments for all edges
 * @var segments_per_edge Number of segments per individual edge
 * @var edge_types Classification types for each edge
 * @var obj_vertices Object-level vertex data
 */

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

/**
 * @brief Tessellate a CAD shape into renderable mesh data
 *
 * Converts an OpenCASCADE TopoDS_Shape into triangulated mesh and polyline
 * representations suitable for web-based 3D visualization. The tessellation
 * process can be configured for quality vs performance tradeoffs.
 *
 * @param shape The input CAD shape to tessellate
 * @param deflection Maximum deviation of tessellation from true geometry
 * @param angular_tolerance Angular tolerance for tessellation quality
 * @param compute_faces Whether to tessellate face geometry
 * @param compute_edges Whether to tessellate edge geometry
 * @param parallel Enable parallel processing for tessellation
 * @param debug Debug output level (0=none, higher=more verbose)
 * @param timeit Enable timing measurements for performance analysis
 * @return MeshData structure containing all tessellated geometry
 */
MeshData tessellate(TopoDS_Shape shape, double deflection, double angular_tolerance,
                    bool compute_faces, bool compute_edges, bool parallel, int debug, bool timeit);
