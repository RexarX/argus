#include <pch.hpp>

#include <pybind11/pybind11.h>

#include <argus/assert.hpp>
#include <argus/sim.hpp>
#include <bind.hpp>

namespace py = pybind11;

void argus::python::bind_sim(py::module_* module) {
    ARGUS_ASSERT(module != nullptr);
    module->doc() = "argus simulation bindings";
    module->def("library_name", &argus::sim::library_name);
}
