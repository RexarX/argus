#include <pch.hpp>

#include <pybind11/pybind11.h>

#include <argus/version.hpp>
#include <bind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(argus, module) {
    module.doc() = "argus";
    module.attr("__path__") = py::list();
    module.attr("__version__") = py::str(argus::version_string());

    argus::python::bind_log(&module);
    argus::python::bind_assert(&module);

#ifdef ARGUS_ENABLE_ALGO
    auto algo = module.def_submodule("algo");
    argus::python::bind_algo(&algo);
#endif

#ifdef ARGUS_ENABLE_SIM
    auto sim = module.def_submodule("sim");
    argus::python::bind_sim(&sim);
#endif
}
