#include "tessellator.h"

auto get_timer() {
    return std::chrono::high_resolution_clock::now();
}

void stop_timer(std::chrono::time_point<std::chrono::high_resolution_clock> start, std::string message) {
    auto done = get_timer();
    float d = (std::chrono::duration_cast<std::chrono::milliseconds>(done - start).count()) / 1000.0;
    std::cout << std::printf("%7.2f",d) << " sec: | | | " << message << std::endl;
}

void log(std::string message) {
    std::cout << message << std::endl;
}

template<typename T>
void log_xyz(std::string msg, T x, T y, T z, bool endline=true) {
    std::cout << msg << ": (" << x << ", " << y << ", " << z << ")" ;
    if (endline) std::cout << std::endl;
}

template<typename T>
py::array_t<T> copy_to_numpy(T *arr, int size) {
    auto array = py::array_t<T>(size);
    auto array_buffer = array.request();
    T *arr_vertices_ptr = (T *) array_buffer.ptr;
    std::memcpy(arr_vertices_ptr, arr, size * sizeof(T));
    return array;
}

GeomAbs_SurfaceType get_face_type(TopoDS_Face face){
    return BRepAdaptor_Surface(face).GetType();
}

GeomAbs_CurveType get_edge_type(TopoDS_Edge edge){
    return BRepAdaptor_Curve(edge).GetType();
}

MeshData collect_mesh_data(
    FaceData face_list[], 
    int num_vertices,
    int num_triangles,
    int num_faces,
    EdgeData edge_list[],
    int num_segments,
    int num_edges,
    Standard_Real obj_vertices[],
    int num_obj_vertices,
    bool timeit
) {
    /*
     * Collect vertices and triangles
     */
    auto start = get_timer();

    auto vertices = new Standard_Real[3 * num_vertices];
    auto normals = new Standard_Real[3 * num_vertices];
    auto triangles = new Standard_Integer[3 * num_triangles];
    auto triangles_per_face = new Standard_Integer[num_faces];
    auto face_types = new Standard_Integer[num_faces];
    auto edge_types = new Standard_Integer[num_edges];

    int v_total = 0;
    int t_total = 0;

    for (int i=0; i<num_faces; i++) {

        auto f = face_list[i];

        auto v = f.vertices;
        auto n = f.normals;
        auto t = f.triangles;

        for (int j=0; j<f.num_vertices*3; j++) {
            vertices[v_total + j] = v[j];
            normals[v_total + j] = n[j];
        }

        for (int j=0; j<f.num_triangles*3; j++) {
            triangles[t_total + j] = t[j];
        }

        triangles_per_face[i] = f.num_triangles;
        face_types[i] = f.face_type;

        v_total += 3 * f.num_vertices;
        t_total += 3 * f.num_triangles;
    }
    if(timeit) stop_timer(start, "Collect vertices and triangles");

    if (timeit) start = get_timer();

    auto segments = new Standard_Real[6 * num_segments];
    auto segments_per_edge = new Standard_Integer[num_edges];
    
    /*
     * Collect segments
     */
    int e_total = 0;

    for (int i = 0; i < num_edges; i++) {
        auto e = edge_list[i];

        for (int j = 0; j < 6 * e.num_segments; j++) {
            segments[e_total + j] = e.segments[j];
        }
        segments_per_edge[i] = e.num_segments;
        edge_types[i] = e.edge_type;

        e_total += 6 * e.num_segments;
    }

    if(timeit) stop_timer(start, "Collect edges");

    if (timeit) start = get_timer();

    auto arr_vertices = copy_to_numpy(vertices, 3 * num_vertices);
    auto arr_normals = copy_to_numpy(normals, 3 * num_vertices);
    auto arr_triangles = copy_to_numpy(triangles, 3 * num_triangles);
    auto arr_triangles_per_face = copy_to_numpy(triangles_per_face, num_faces);
    auto arr_face_types = copy_to_numpy(face_types, num_faces);
    auto arr_edge_types = copy_to_numpy(edge_types, num_edges);
    auto arr_obj_vertices = copy_to_numpy(obj_vertices, 3 * num_obj_vertices);
    auto arr_segments = copy_to_numpy(segments, 6 * num_segments);
    auto arr_segments_per_edge = copy_to_numpy(segments_per_edge, num_edges);

    if(timeit) stop_timer(start, "Cast to numpy");

    if(timeit) start = get_timer(); 
    MeshData mesh_data = {
        arr_vertices,
        arr_normals,
        arr_triangles,
        arr_triangles_per_face,
        arr_face_types,
        arr_segments,
        arr_segments_per_edge,
        arr_edge_types,
        arr_obj_vertices
    };

    delete[] vertices;
    delete[] normals;
    delete[] triangles;
    delete[] triangles_per_face;
    delete[] segments;
    delete[] segments_per_edge;

    if(timeit) stop_timer(start, "Create result class");
    return mesh_data;    
}

MeshData tessellate(TopoDS_Shape shape, double deflection, double angular_tolerance, 
                    bool compute_faces, bool compute_edges, bool parallel, int debug, bool timeit) {
    /*
     * Tessellate mesh
     */

    // https://dev.opencascade.org/node/81262#comment-21130
    // BRepTools::Clean(shape);

    auto start = get_timer();

    if (compute_edges || compute_faces) {
        BRepMesh_IncrementalMesh mesher (shape, deflection, Standard_False, angular_tolerance, parallel);    
        if(timeit) stop_timer(start, "Computing BRep incremental mesh");
    }
    TopLoc_Location loc;

    int num_faces = 0;
    FaceData* face_list = new FaceData[num_faces];

    int total_num_vertices = 0;
    int total_num_triangles = 0;
    
    if (compute_faces) {
        if(timeit) start = get_timer();
        
        TopTools_IndexedMapOfShape face_map = TopTools_IndexedMapOfShape();
        TopExp::MapShapes(shape, TopAbs_FACE, face_map);

        num_faces = face_map.Extent();
        face_list = new FaceData[num_faces];

        try {
            long offset = -1;
            // long triangle_count = 0;

            // long s = 0;
            for (int i = 0; i < num_faces; i++) {
                if (debug == 2) std::cout << "face " << i << std::endl;

                const TopoDS_Face& topods_face = TopoDS::Face(face_map.FindKey(i+1));

                TopAbs_Orientation orient = topods_face.Orientation();
                Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(topods_face, loc);

                if (! triangulation.IsNull()) {
                    const Standard_Integer num_nodes = triangulation->NbNodes();
                    const Standard_Integer num_triangles = triangulation->NbTriangles();
                    
                    face_list[i].vertices = new Standard_Real[num_nodes * 3];
                    face_list[i].normals = new Standard_Real[num_nodes * 3];
                    face_list[i].triangles = new Standard_Integer[num_triangles * 3];

                    BRepGProp_Face prop(topods_face);

                    for (Standard_Integer j = 0; j < num_nodes; j++) {
                        gp_Pnt point = triangulation->Node(j+1).Transformed(loc).XYZ();
                        face_list[i].vertices[3 * j    ] = point.X();
                        face_list[i].vertices[3 * j + 1] = point.Y();
                        face_list[i].vertices[3 * j + 2] = point.Z();
                        
                        if (debug == 2) log_xyz("vertex", point.X(), point.Y(), point.Z(), false);

                        if (triangulation->HasUVNodes()) {
                            const gp_Pnt2d& uv = triangulation->UVNode(j+1);
                            gp_Pnt point; 
                            gp_Vec normal;
                            prop.Normal(uv.X(), uv.Y(), point, normal);
                            if (normal.SquareMagnitude() > 0.0) normal.Normalize();
                            if (topods_face.Orientation() == TopAbs_INTERNAL) normal.Reverse();

                            face_list[i].normals[3 * j    ] = normal.X();
                            face_list[i].normals[3 * j + 1] = normal.Y();
                            face_list[i].normals[3 * j + 2] = normal.Z();

                            if (debug == 2) log_xyz(" normal", normal.X(), normal.Y(), normal.Z());
                        }
                    }

                    for (Standard_Integer j = 0; j < num_triangles; j++) {
                        Standard_Integer index0, index1, index2;
                        triangulation->Triangle(j+1).Get(index0, index1, index2);
                        
                        face_list[i].triangles[3 * j    ] = offset + index0;
                        face_list[i].triangles[3 * j + 1] = offset + ((orient == TopAbs_REVERSED) ? index2 : index1);
                        face_list[i].triangles[3 * j + 2] = offset + ((orient == TopAbs_REVERSED) ? index1 : index2);

                        if (debug == 2) log_xyz("triangle ", offset + index0, 
                            offset + ((orient == TopAbs_REVERSED) ? index2 : index1),
                            offset + ((orient == TopAbs_REVERSED) ? index1 : index2)
                        );
                    }

                    // triangle_count += num_triangles * 3;
                    
                    face_list[i].num_vertices = num_nodes;
                    face_list[i].num_triangles = num_triangles;
                    face_list[i].face_type = get_face_type(topods_face);


                    offset += num_nodes;     
                    total_num_vertices += num_nodes;
                    total_num_triangles += num_triangles;
                } else {
                    if (debug == 1) std::cerr << "=> warning: Triangulation is null for face " << i << std::endl;
                    face_list[i].vertices = nullptr;
                    face_list[i].normals = nullptr;
                    face_list[i].triangles = nullptr;
                    face_list[i].num_vertices = 0;
                    face_list[i].num_triangles = 0;
                    face_list[i].face_type = -1;
                }
            }
        } catch (Standard_Failure& e) {
            std::cerr << "=> Standard_Failure: " << e.GetMessageString() << std::endl;
        } catch (...) {
            std::cerr << "=> Unknown exception caught" << std::endl;
        }
        if(timeit) stop_timer(start, "Computing tessellation");
    }
    /*
     * Compute edges
     */

    
    int num_edges = 0;

    int total_num_segments = 0;
    EdgeData* edge_list = new EdgeData[num_edges];
    
    if (compute_edges) {
        if(timeit) start = get_timer();

        TopTools_IndexedMapOfShape edge_map = TopTools_IndexedMapOfShape();
        TopTools_IndexedDataMapOfShapeListOfShape ancestor_map = TopTools_IndexedDataMapOfShapeListOfShape();

        TopExp::MapShapes(shape, TopAbs_EDGE, edge_map);
        TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, ancestor_map);
        
        num_edges = edge_map.Extent();
        edge_list = new EdgeData[num_edges];
        
        // int edges_offset = 0;
        for (int i=0; i<num_edges; i++) {
            const TopTools_ListOfShape& face_list = ancestor_map.FindFromIndex(i+1);

            if (face_list.Extent() > 0){

                const TopoDS_Face& topods_face = TopoDS::Face(face_list.First());
                const TopoDS_Edge& topods_edge = TopoDS::Edge(edge_map(i+1));

                TopLoc_Location loc = TopLoc_Location();

                Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(topods_face, loc);
                Handle(Poly_PolygonOnTriangulation) poly = BRep_Tool::PolygonOnTriangulation(topods_edge, triangulation, loc);

                if (!poly.IsNull()){
                    int num_nodes = poly->NbNodes();
                    
                    edge_list[i].segments = new Standard_Real[6 * (num_nodes - 1)];

                    for (int j=0; j<num_nodes - 1; j++){
                        gp_Pnt p1 = triangulation->Node(poly->Node(j+1)).Transformed(loc).Coord();
                        gp_Pnt p2 = triangulation->Node(poly->Node(j+2)).Transformed(loc).Coord();
                        edge_list[i].segments[j * 6  + 0] = p1.X();
                        edge_list[i].segments[j * 6  + 1] = p1.Y();
                        edge_list[i].segments[j * 6  + 2] = p1.Z();
                        edge_list[i].segments[j * 6  + 3] = p2.X();
                        edge_list[i].segments[j * 6  + 4] = p2.Y();
                        edge_list[i].segments[j * 6  + 5] = p2.Z();
                    }

                    total_num_segments += (num_nodes - 1);
                    edge_list[i].num_segments = num_nodes - 1;
                    edge_list[i].edge_type = get_edge_type(topods_edge);

                } else {
                    if (debug == 1) std::cerr << "=> warning: no face polygon for egde " << i << std::endl;
                    edge_list[i].segments = nullptr;
                    edge_list[i].num_segments = 0;
                    edge_list[i].edge_type = -1;
                }
            } else {
                if (debug == 1) std::cerr << "=> warning: no face ancestors for egde " << i << std::endl;
                edge_list[i].segments = nullptr;
                edge_list[i].num_segments = 0;
                edge_list[i].edge_type = -1;
            }
        }
        if(timeit) stop_timer(start, "Computing edges");
    }

    /*
     * Collect vertices
     */


    if(timeit) start = get_timer();

    TopTools_IndexedMapOfShape vertex_map = TopTools_IndexedMapOfShape();
    TopExp::MapShapes(shape, TopAbs_VERTEX, vertex_map);

    int num_vertices = vertex_map.Extent();

    Standard_Real* vertex_list = new Standard_Real[3 * num_vertices];
    for (int i = 0; i < num_vertices; i++) {
        const TopoDS_Vertex& topods_vertex = TopoDS::Vertex(vertex_map.FindKey(i+1));
        gp_Pnt p = BRep_Tool::Pnt(topods_vertex);
        vertex_list[3 * i + 0] = p.X();
        vertex_list[3 * i + 1] = p.Y(); 
        vertex_list[3 * i + 2] = p.Z();
    }

    if(timeit) stop_timer(start, "Computing vertices");

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
        timeit
    );

    // delete face_list and edge_list
    for (int i=0; i<num_faces; i++) {
        delete[] face_list[i].vertices;
        delete[] face_list[i].normals;
        delete[] face_list[i].triangles;
    }
    delete[] face_list;

    for (int i=0; i<num_edges; i++) {
        delete[] edge_list[i].segments;
    }
    delete[] edge_list;
    delete[] vertex_list;

    return result;
}

void register_tessellator(pybind11::module_ &m_gbl) {
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
        )pbdoc"
    );
}
