/**
 * @file ocp_utils.h
 * @brief Utilities for cross-module type sharing with OCP on Windows
 *
 * On Windows, pybind11 uses pointer comparison for std::type_info, causing
 * type mismatch errors when sharing types between OCP and ocp_addons modules.
 * These utilities bypass pybind11's type registry by directly accessing the
 * C++ object pointer from pybind11's internal instance structure.
 */

#ifndef OCP_UTILS_H
#define OCP_UTILS_H

#include <pybind11/pybind11.h>
#include <pybind11/detail/internals.h>

#include <TopoDS_Shape.hxx>
#include <TopLoc_Location.hxx>

namespace py = pybind11;

namespace ocp_utils {

/**
 * @brief Extract TopoDS_Shape pointer from a pybind11-wrapped OCP object
 * @param obj Python object wrapping a TopoDS_Shape
 * @return Pointer to the underlying C++ TopoDS_Shape
 * @throws py::type_error if pointer extraction fails
 */
inline TopoDS_Shape* extract_shape(py::handle obj) {
    auto *inst = reinterpret_cast<py::detail::instance *>(obj.ptr());
    void *ptr = inst->simple_value_holder[0];
    if (ptr) {
        return reinterpret_cast<TopoDS_Shape*>(ptr);
    }
    throw py::type_error("Could not extract TopoDS_Shape pointer from object");
}

/**
 * @brief Extract TopLoc_Location pointer from a pybind11-wrapped OCP object
 * @param obj Python object wrapping a TopLoc_Location
 * @return Pointer to the underlying C++ TopLoc_Location
 * @throws py::type_error if pointer extraction fails
 */
inline TopLoc_Location* extract_location(py::handle obj) {
    auto *inst = reinterpret_cast<py::detail::instance *>(obj.ptr());
    void *ptr = inst->simple_value_holder[0];
    if (ptr) {
        return reinterpret_cast<TopLoc_Location*>(ptr);
    }
    throw py::type_error("Could not extract TopLoc_Location pointer from object");
}

/**
 * @brief Create an OCP Python object wrapping a TopoDS_Shape
 * @param shape The C++ TopoDS_Shape to wrap
 * @return Python object of type OCP.TopoDS.TopoDS_Shape
 * @throws py::type_error if object creation fails
 */
inline py::object create_shape(const TopoDS_Shape &shape) {
    py::object ocp_topoDS = py::module_::import("OCP.TopoDS");
    py::object shape_class = ocp_topoDS.attr("TopoDS_Shape");
    py::object new_obj = shape_class();

    auto *inst = reinterpret_cast<py::detail::instance *>(new_obj.ptr());
    void *ptr = inst->simple_value_holder[0];
    if (ptr) {
        *reinterpret_cast<TopoDS_Shape*>(ptr) = shape;
        return new_obj;
    }
    throw py::type_error("Could not access TopoDS_Shape storage in OCP object");
}

/**
 * @brief Create an OCP Python object wrapping a TopLoc_Location
 * @param location The C++ TopLoc_Location to wrap
 * @return Python object of type OCP.TopLoc.TopLoc_Location
 * @throws py::type_error if object creation fails
 */
inline py::object create_location(const TopLoc_Location &location) {
    py::object ocp_topLoc = py::module_::import("OCP.TopLoc");
    py::object loc_class = ocp_topLoc.attr("TopLoc_Location");
    py::object new_obj = loc_class();

    auto *inst = reinterpret_cast<py::detail::instance *>(new_obj.ptr());
    void *ptr = inst->simple_value_holder[0];
    if (ptr) {
        *reinterpret_cast<TopLoc_Location*>(ptr) = location;
        return new_obj;
    }
    throw py::type_error("Could not access TopLoc_Location storage in OCP object");
}

} // namespace ocp_utils

#endif // OCP_UTILS_H
