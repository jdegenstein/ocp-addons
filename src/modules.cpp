#include <pybind11/pybind11.h>

void register_serializer(pybind11::module_ &);
void register_tessellator(pybind11::module_ &);

PYBIND11_MODULE(ocp_addons, m) {
    m.doc() = R"pbdoc(
        OCP addons
        ----------

        .. currentmodule:: ocp_addons

        .. autosummary::
           :toctree: _generate
    )pbdoc";

    register_serializer(m);
    register_tessellator(m);
}
