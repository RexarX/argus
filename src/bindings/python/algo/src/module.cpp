#include <pch.hpp>

#include <pybind11/pybind11.h>

#include <argus/algo.hpp>
#include <argus/assert.hpp>
#include <bind.hpp>

namespace py = pybind11;

void argus::python::bind_algo(py::module_* module) {
    ARGUS_ASSERT(module != nullptr);
    module->doc() = "argus algorithm bindings";
    module->def("library_name", &argus::library_name);
}
