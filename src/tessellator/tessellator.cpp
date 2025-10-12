#include "tessellator.h"
#include "utils.h"

namespace py = pybind11;

GeomAbs_SurfaceType get_face_type(TopoDS_Face face)
{
    return BRepAdaptor_Surface(face).GetType();
}

GeomAbs_CurveType get_edge_type(TopoDS_Edge edge)
{
    return BRepAdaptor_Curve(edge).GetType();
}

/**
 * @brief Collects and processes mesh data from face and edge lists into a unified MeshData structure.
 *
 * This function aggregates vertex, triangle, and edge data from multiple faces and edges into
 * consolidated arrays. It can optionally compute missing vertex normals using face normal
 * interpolation and generate triangle edges when edge data is not provided.
 *
 * @param face_list Array of FaceData structures containing face geometry information
 * @param num_vertices Total number of vertices across all faces
 * @param num_triangles Total number of triangles across all faces
 * @param num_faces Number of faces in the face_list array
 * @param edge_list Array of EdgeData structures containing edge segment information
 * @param num_segments Total number of edge segments across all edges
 * @param num_edges Number of edges in the edge_list array
 * @param obj_vertices Array of additional object vertices (external geometry)
 * @param num_obj_vertices Number of vertices in obj_vertices array
 * @param compute_missing_normals If true, computes vertex normals by interpolating face normals
 * @param compute_missing_edges If true, generates edge segments from triangle edges when edge data is unavailable
 * @param timeit If true, enables timing measurements for performance profiling
 *
 * @return MeshData structure containing consolidated mesh geometry with numpy-wrapped arrays
 *
 * @details The function performs the following operations:
 * - Consolidates vertices, normals, and triangles from all faces into unified arrays
 * - Optionally computes vertex normals by averaging adjacent face normals and normalizing
 * - Collects edge segments from provided edge data or generates them from triangle edges
 * - Converts all floating-point data from double to float precision
 * - Wraps all arrays in numpy-compatible format with automatic memory management
 * - Tracks triangles and segments per face/edge for proper indexing
 * - Includes timing measurements for performance analysis when enabled
 *
 * @note Memory for intermediate arrays is automatically cleaned up after numpy wrapping.
 *       The returned MeshData uses capsules for proper Python memory management.
 */

MeshData collect_mesh_data(
    FaceData face_list[],
    int num_vertices,
    int num_triangles,
    int num_faces,
    EdgeData edge_list[],
    int num_segments,
    int num_edges,
    double obj_vertices[],
    int num_obj_vertices,
    bool compute_missing_normals,
    bool compute_missing_edges,
    bool timeit)
{
    /*
     * Collect vertices and triangles
     */

    Timer timer("Collect vertices and triangles", 2, timeit);

    auto vertices = new double[3 * num_vertices];
    auto normals = new double[3 * num_vertices];
    auto triangles = new int[3 * num_triangles];
    auto triangles_per_face = new int[num_faces];
    auto face_types = new int[num_faces];
    auto edge_types = new int[num_edges];

    int v_total = 0;
    int t_total = 0;

    for (int i = 0; i < num_faces; i++)
    {

        auto f = face_list[i];

        auto v = f.vertices;
        auto n = f.normals;
        auto t = f.triangles;

        for (int j = 0; j < f.num_vertices * 3; j++)
        {
            vertices[v_total + j] = v[j];
            normals[v_total + j] = n[j];
        }

        for (int j = 0; j < f.num_triangles * 3; j++)
        {
            triangles[t_total + j] = t[j];
        }

        triangles_per_face[i] = f.num_triangles;
        face_types[i] = f.face_type;

        v_total += 3 * f.num_vertices;
        t_total += 3 * f.num_triangles;
    }
    if (compute_missing_normals)
    {
        timer.reset("Interpolating normals", 2);

        for (int i = 0; i < 3 * num_vertices; i++)
        {
            normals[i] = 0.0;
        }

        for (int i = 0; i < num_triangles; i++)
        {
            double c0_0 = (vertices[3 * triangles[3 * i + 0] + 0]);
            double c0_1 = (vertices[3 * triangles[3 * i + 0] + 1]);
            double c0_2 = (vertices[3 * triangles[3 * i + 0] + 2]);
            double c1_0 = (vertices[3 * triangles[3 * i + 1] + 0]);
            double c1_1 = (vertices[3 * triangles[3 * i + 1] + 1]);
            double c1_2 = (vertices[3 * triangles[3 * i + 1] + 2]);
            double c2_0 = (vertices[3 * triangles[3 * i + 2] + 0]);
            double c2_1 = (vertices[3 * triangles[3 * i + 2] + 1]);
            double c2_2 = (vertices[3 * triangles[3 * i + 2] + 2]);
            // c2 - c1
            double v1_0 = (c2_0 - c1_0);
            double v1_1 = (c2_1 - c1_1);
            double v1_2 = (c2_2 - c1_2);
            // c0 - c1
            double v2_0 = (c0_0 - c1_0);
            double v2_1 = (c0_1 - c1_1);
            double v2_2 = (c0_2 - c1_2);
            // cross product of v1 and v2
            double n_0 = (v1_1 * v2_2 - v1_2 * v2_1);
            double n_1 = (v1_2 * v2_0 - v1_0 * v2_2);
            double n_2 = (v1_0 * v2_1 - v1_1 * v2_0);
            // interpolate vertex normal by blending all face normals of a vertex
            for (int j = 0; j < 3; j++)
            {
                normals[3 * triangles[3 * i + j] + 0] += n_0;
                normals[3 * triangles[3 * i + j] + 1] += n_1;
                normals[3 * triangles[3 * i + j] + 2] += n_2;
            }
        }
        // and normalize later
        for (int i = 0; i < num_vertices; i++)
        {
            double norm = sqrt(normals[3 * i] * normals[3 * i] + normals[3 * i + 1] * normals[3 * i + 1] + normals[3 * i + 2] * normals[3 * i + 2]);
            normals[3 * i] /= norm;
            normals[3 * i + 1] /= norm;
            normals[3 * i + 2] /= norm;
        }
    }

    auto segments = new double[6 * num_segments];
    auto segments_per_edge = new int[num_edges];

    /*
     * Collect segments
     */
    int e_total = 0;

    if (compute_missing_edges)
    {
        timer.reset("Compute missing edges", 2);

        num_edges = num_triangles;
        num_segments = 3 * num_triangles;
        segments = new double[18 * num_edges];
        segments_per_edge = new int[num_edges];

        for (int i = 0; i < num_triangles; i++)
        {
            double c0_0 = vertices[3 * triangles[3 * i + 0] + 0];
            double c0_1 = vertices[3 * triangles[3 * i + 0] + 1];
            double c0_2 = vertices[3 * triangles[3 * i + 0] + 2];
            double c1_0 = vertices[3 * triangles[3 * i + 1] + 0];
            double c1_1 = vertices[3 * triangles[3 * i + 1] + 1];
            double c1_2 = vertices[3 * triangles[3 * i + 1] + 2];
            double c2_0 = vertices[3 * triangles[3 * i + 2] + 0];
            double c2_1 = vertices[3 * triangles[3 * i + 2] + 1];
            double c2_2 = vertices[3 * triangles[3 * i + 2] + 2];
            segments[e_total + 0] = c0_0;
            segments[e_total + 1] = c0_1;
            segments[e_total + 2] = c0_2;
            segments[e_total + 3] = c1_0;
            segments[e_total + 4] = c1_1;
            segments[e_total + 5] = c1_2;
            segments[e_total + 6] = c1_0;
            segments[e_total + 7] = c1_1;
            segments[e_total + 8] = c1_2;
            segments[e_total + 9] = c2_0;
            segments[e_total + 10] = c2_1;
            segments[e_total + 11] = c2_2;
            segments[e_total + 12] = c2_0;
            segments[e_total + 13] = c2_1;
            segments[e_total + 14] = c2_2;
            segments[e_total + 15] = c0_0;
            segments[e_total + 16] = c0_1;
            segments[e_total + 17] = c0_2;
            e_total += 18;
            segments_per_edge[i] = 3;
        }
    }
    else
    {
        timer.reset("Collecting edges", 2);
        for (int i = 0; i < num_edges; i++)
        {
            auto e = edge_list[i];

            for (int j = 0; j < 6 * e.num_segments; j++)
            {
                segments[e_total + j] = e.segments[j];
            }
            segments_per_edge[i] = e.num_segments;
            edge_types[i] = e.edge_type;

            e_total += 6 * e.num_segments;
        }
    }

    timer.reset("Create MeshData", 2);

    MeshData mesh_data;

    timer.reset("Cast to float", 2);

    float *vertices32 = convert_to_float(vertices, 3 * num_vertices);
    float *normals32 = convert_to_float(normals, 3 * num_vertices);
    float *obj_vertices32 = convert_to_float(obj_vertices, 3 * num_obj_vertices);
    float *segments32 = convert_to_float(segments, 6 * num_segments);

    timer.reset("Cast to numpy", 2);

    // wrap_numpy use a capsule, so Python triggers deletion
    mesh_data.vertices = wrap_numpy(vertices32, 3 * num_vertices);
    mesh_data.normals = wrap_numpy(normals32, 3 * num_vertices);
    mesh_data.triangles = wrap_numpy(triangles, 3 * num_triangles);
    mesh_data.triangles_per_face = wrap_numpy(triangles_per_face, num_faces);
    mesh_data.face_types = wrap_numpy(face_types, num_faces);
    mesh_data.edge_types = wrap_numpy(edge_types, num_edges);
    mesh_data.obj_vertices = wrap_numpy(obj_vertices32, 3 * num_obj_vertices);
    mesh_data.segments = wrap_numpy(segments32, 6 * num_segments);
    mesh_data.segments_per_edge = wrap_numpy(segments_per_edge, num_edges);

    delete[] vertices;
    delete[] obj_vertices;
    delete[] normals;
    delete[] segments;

    timer.stop();

    return mesh_data;
}

/**
 * @brief Tessellates a TopoDS_Shape into a mesh representation with vertices, triangles, and edges.
 *
 * This function performs mesh tessellation on an OpenCascade TopoDS_Shape object, generating
 * triangulated faces, edge segments, and vertex data. It uses BRepMesh_IncrementalMesh for
 * the underlying tessellation and supports parallel processing.
 *
 * @param shape The TopoDS_Shape object to tessellate
 * @param deflection Maximum allowed deviation between the original surface and the tessellated mesh
 * @param angular_tolerance Angular tolerance for tessellation in radians
 * @param compute_faces Whether to compute face triangulation data
 * @param compute_edges Whether to compute edge segment data
 * @param parallel Whether to enable parallel processing during tessellation
 * @param debug Debug level for logging (0 = no debug output)
 * @param timeit Whether to measure and report timing information
 *
 * @return MeshData structure containing:
 *         - vertices: Array of vertex coordinates (x,y,z)
 *         - normals: Array of vertex normals (if available)
 *         - triangles: Array of triangle indices
 *         - triangles_per_face: Number of triangles per face
 *         - face_types: Type classification for each face
 *         - segments: Array of edge segment coordinates
 *         - segments_per_edge: Number of segments per edge
 *         - edge_types: Type classification for each edge
 *         - obj_vertices: Original shape vertices
 *
 * @note The function handles orientation correction for reversed faces and computes normals
 *       when UV nodes are available in the triangulation. Edge processing requires face
 *       ancestors to be present.
 *
 * @warning Memory allocation is performed for face and edge data arrays. Proper cleanup
 *          should be handled by the MeshData structure or calling code.
 */

MeshData tessellate(py::object obj, double deflection, double angular_tolerance,
                    bool compute_faces, bool compute_edges, bool parallel, int debug, bool timeit)
{
    /*
     * Tessellate mesh
     */

    auto *shape_ptr = obj.cast<TopoDS_Shape *>();
    const TopoDS_Shape &shape = *shape_ptr;

    Logger logger(debug);
    Timer overall("Overall", 0, timeit);
    Timer timer;

    if (compute_edges || compute_faces)
    {
        logger.info("deflection", deflection, "angular_tolerance", angular_tolerance, "parallel", parallel);
        timer.start("Computing BRep incremental mesh", 1, timeit);

        // https://dev.opencascade.org/node/81262#comment-21130
        // BRepTools::Clean(shape);
        BRepMesh_IncrementalMesh mesher(shape, deflection, Standard_False, angular_tolerance, parallel);
        logger.debug("IsDone", mesher.IsDone());
        logger.debug("GetStatusFlags", mesher.GetStatusFlags());

        timer.stop();
    }
    TopLoc_Location loc;

    int has_normals = false; // assumption: if one face has no normal, no faces has normals
    int num_faces = 0;
    FaceData *face_list;

    int total_num_vertices = 0;
    int total_num_triangles = 0;

    if (compute_faces)
    {
        timer.start("Computing tessellation", 1, timeit);

        TopTools_IndexedMapOfShape face_map = TopTools_IndexedMapOfShape();
        TopExp::MapShapes(shape, TopAbs_FACE, face_map);

        num_faces = face_map.Extent();
        face_list = new FaceData[num_faces];

        logger.debug("num_faces", num_faces);

        try
        {
            long offset = -1;

            for (int i = 0; i < num_faces; i++)
            {
                const TopoDS_Face &topods_face = TopoDS::Face(face_map.FindKey(i + 1));

                TopAbs_Orientation orient = topods_face.Orientation();
                Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(topods_face, loc);

                if (!triangulation.IsNull())
                {
                    const Standard_Integer num_nodes = triangulation->NbNodes();
                    const Standard_Integer num_triangles = triangulation->NbTriangles();

                    face_list[i].vertices = new Standard_Real[num_nodes * 3];
                    face_list[i].normals = new Standard_Real[num_nodes * 3];
                    face_list[i].triangles = new Standard_Integer[num_triangles * 3];

                    BRepGProp_Face prop(topods_face);

                    for (Standard_Integer j = 0; j < num_nodes; j++)
                    {
                        gp_Pnt point = triangulation->Node(j + 1).Transformed(loc).XYZ();
                        face_list[i].vertices[3 * j] = point.X();
                        face_list[i].vertices[3 * j + 1] = point.Y();
                        face_list[i].vertices[3 * j + 2] = point.Z();

                        logger.trace_xyz("vertex", point.X(), point.Y(), point.Z(), false);

                        if (triangulation->HasUVNodes())
                        {
                            has_normals = true;
                            const gp_Pnt2d &uv = triangulation->UVNode(j + 1);
                            gp_Pnt point;
                            gp_Vec normal;
                            prop.Normal(uv.X(), uv.Y(), point, normal);
                            if (normal.SquareMagnitude() > 0.0)
                                normal.Normalize();
                            if (topods_face.Orientation() == TopAbs_INTERNAL)
                                normal.Reverse();

                            face_list[i].normals[3 * j] = normal.X();
                            face_list[i].normals[3 * j + 1] = normal.Y();
                            face_list[i].normals[3 * j + 2] = normal.Z();

                            logger.trace_xyz(" normal", normal.X(), normal.Y(), normal.Z(), false);
                        }
                    }

                    for (Standard_Integer j = 0; j < num_triangles; j++)
                    {
                        Standard_Integer index0, index1, index2;
                        triangulation->Triangle(j + 1).Get(index0, index1, index2);

                        face_list[i].triangles[3 * j] = offset + index0;
                        face_list[i].triangles[3 * j + 1] = offset + ((orient == TopAbs_REVERSED) ? index2 : index1);
                        face_list[i].triangles[3 * j + 2] = offset + ((orient == TopAbs_REVERSED) ? index1 : index2);

                        logger.trace_xyz("triangle ", offset + index0,
                                         offset + ((orient == TopAbs_REVERSED) ? index2 : index1),
                                         offset + ((orient == TopAbs_REVERSED) ? index1 : index2), false);
                    }

                    // triangle_count += num_triangles * 3;

                    face_list[i].num_vertices = num_nodes;
                    face_list[i].num_triangles = num_triangles;
                    face_list[i].face_type = get_face_type(topods_face);

                    offset += num_nodes;
                    total_num_vertices += num_nodes;
                    total_num_triangles += num_triangles;
                }
                else
                {
                    logger.info("=> warning: Triangulation is null for face ", i, "\n");

                    face_list[i].vertices = nullptr;
                    face_list[i].normals = nullptr;
                    face_list[i].triangles = nullptr;
                    face_list[i].num_vertices = 0;
                    face_list[i].num_triangles = 0;
                    face_list[i].face_type = -1;
                }
            }
        }
        catch (Standard_Failure &e)
        {
            py::print("Error:", e.GetMessageString(), "\n");
        }
        catch (...)
        {
            py::print("Error: unknown\n");
        }

        timer.stop();
    }

    /*
     * Compute edges
     */

    int num_edges = 0;

    int total_num_segments = 0;
    EdgeData *edge_list = new EdgeData[num_edges];

    if (compute_edges)
    {
        timer.start("Computing edges", 1, timeit);

        TopTools_IndexedMapOfShape edge_map = TopTools_IndexedMapOfShape();
        TopTools_IndexedDataMapOfShapeListOfShape ancestor_map = TopTools_IndexedDataMapOfShapeListOfShape();

        TopExp::MapShapes(shape, TopAbs_EDGE, edge_map);
        TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, ancestor_map);

        num_edges = edge_map.Extent();
        edge_list = new EdgeData[num_edges];

        // int edges_offset = 0;
        for (int i = 0; i < num_edges; i++)
        {
            const TopTools_ListOfShape &face_list = ancestor_map.FindFromIndex(i + 1);

            if (face_list.Extent() > 0)
            {

                const TopoDS_Face &topods_face = TopoDS::Face(face_list.First());
                const TopoDS_Edge &topods_edge = TopoDS::Edge(edge_map(i + 1));

                TopLoc_Location loc = TopLoc_Location();

                Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(topods_face, loc);
                Handle(Poly_PolygonOnTriangulation) poly = BRep_Tool::PolygonOnTriangulation(topods_edge, triangulation, loc);

                if (!poly.IsNull())
                {
                    int num_nodes = poly->NbNodes();

                    edge_list[i].segments = new Standard_Real[6 * (num_nodes - 1)];

                    for (int j = 0; j < num_nodes - 1; j++)
                    {
                        gp_Pnt p1 = triangulation->Node(poly->Node(j + 1)).Transformed(loc).Coord();
                        gp_Pnt p2 = triangulation->Node(poly->Node(j + 2)).Transformed(loc).Coord();
                        edge_list[i].segments[j * 6 + 0] = p1.X();
                        edge_list[i].segments[j * 6 + 1] = p1.Y();
                        edge_list[i].segments[j * 6 + 2] = p1.Z();
                        edge_list[i].segments[j * 6 + 3] = p2.X();
                        edge_list[i].segments[j * 6 + 4] = p2.Y();
                        edge_list[i].segments[j * 6 + 5] = p2.Z();
                    }

                    total_num_segments += (num_nodes - 1);
                    edge_list[i].num_segments = num_nodes - 1;
                    edge_list[i].edge_type = get_edge_type(topods_edge);
                }
                else
                {
                    logger.debug("=> warning: no face polygon for egde ", i);
                    edge_list[i].segments = nullptr;
                    edge_list[i].num_segments = 0;
                    edge_list[i].edge_type = -1;
                }
            }
            else
            {
                logger.debug("=> warning: no face ancestors for egde ", i);
                edge_list[i].segments = nullptr;
                edge_list[i].num_segments = 0;
                edge_list[i].edge_type = -1;
            }
        }
        timer.stop();
    }

    /*
     * Collect vertices
     */

    timer.start("Computing vertices", 1, timeit);

    TopTools_IndexedMapOfShape vertex_map = TopTools_IndexedMapOfShape();
    TopExp::MapShapes(shape, TopAbs_VERTEX, vertex_map);

    int num_vertices = vertex_map.Extent();

    double *vertex_list = new double[3 * num_vertices];
    for (int i = 0; i < num_vertices; i++)
    {
        const TopoDS_Vertex &topods_vertex = TopoDS::Vertex(vertex_map.FindKey(i + 1));
        gp_Pnt p = BRep_Tool::Pnt(topods_vertex);
        vertex_list[3 * i + 0] = p.X();
        vertex_list[3 * i + 1] = p.Y();
        vertex_list[3 * i + 2] = p.Z();
    }

    timer.reset("Collecting mesh data", 1);

    // Return the struct
    auto result = collect_mesh_data(
        face_list,
        total_num_vertices,
        total_num_triangles,
        num_faces,
        edge_list,
        total_num_segments,
        num_edges,
        vertex_list,
        num_vertices,
        !has_normals,                             // interpolate normals
        compute_edges ? (num_edges == 0) : false, // calculate all triangles edges
        timeit);

    timer.stop();
    overall.stop();

    logger.debug("vertices", result.vertices, result.vertices.dtype());
    logger.debug("normals", result.normals, result.normals.dtype());
    logger.debug("triangles", result.triangles, result.triangles.dtype());
    logger.debug("triangles_per_face", result.triangles_per_face, result.triangles_per_face.dtype());
    logger.debug("face_types", result.face_types, result.face_types.dtype());
    logger.debug("segments", result.segments, result.segments.dtype());
    logger.debug("segments_per_edge", result.segments_per_edge, result.segments_per_edge.dtype());
    logger.debug("edge_types", result.edge_types, result.edge_types.dtype());
    logger.debug("obj_vertices", result.obj_vertices, result.obj_vertices.dtype());

    return result;
}

/**
 * @brief Registers the tessellator module with pybind11
 *
 * This function creates a submodule called "tessellator" within the global module
 * and registers the MeshData class along with the tessellate function for Python binding.
 *
 * The MeshData class exposes read-only properties for mesh data including:
 * - vertices: Vertex coordinates
 * - normals: Normal vectors
 * - triangles: Triangle indices
 * - face_types: Types of faces
 * - triangles_per_face: Number of triangles per face
 * - segments: Line segments
 * - segments_per_edge: Number of segments per edge
 * - edge_types: Types of edges
 * - obj_vertices: Object vertices
 */
void register_tessellator(pybind11::module_ &m_gbl)
{
    auto m = m_gbl.def_submodule("tessellator");

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
        OCP Tessellator
        ---------------

        .. currentmodule:: ocp_addons

        .. autosummary::
           :toctree: _generate
    )pbdoc";

    m.def(
        "tessellate",
        &tessellate,
        py::arg("shape"),
        py::arg("deflection"),
        py::arg("angular_tolerance") = 0.3,
        py::arg("compute_faces") = true,
        py::arg("compute_edges") = true,
        py::arg("parallel") = true,
        py::arg("debug") = 0,
        py::arg("timeit") = false,
        R"pbdoc(
        Tessellate a shape

        Tessellate OCP object with a native function via pybind11 and arrow
        )pbdoc");
}
