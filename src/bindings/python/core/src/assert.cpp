#include <pch.hpp>

#include <pybind11/pybind11.h>

#include <argus/assert.hpp>
#include <bind.hpp>

#include <source_location>
#include <string_view>

namespace py = pybind11;

namespace {

py::function& python_assertion_handler() {
    static py::function handler;
    return handler;
}

void python_assertion_thunk(std::string_view condition,
                            std::string_view message,
                            const std::source_location& loc) noexcept {
    if (!python_assertion_handler()) {
        return;
    }

    try {
        py::gil_scoped_acquire gil;
        const char* file = loc.file_name() != nullptr ? loc.file_name() : "";
        python_assertion_handler()(py::str(condition.data(), condition.size()),
                                   py::str(message.data(), message.size()),
                                   py::str(file), loc.line());
    } catch (...) {
    }
}

}  // namespace

void argus::python::bind_assert(py::module_* module) {
    ARGUS_ASSERT(module != nullptr);

    module->def("set_assertion_handler", [](py::object callback) {
        if (callback.is_none()) {
            python_assertion_handler() = py::function();
            argus::set_assertion_handler(nullptr);
            return;
        }
        python_assertion_handler() = py::function(callback);
        argus::set_assertion_handler(&python_assertion_thunk);
    });

    module->def("reset_assertion_handler", []() {
        python_assertion_handler() = py::function();
        argus::set_assertion_handler(nullptr);
    });

    module->def("get_assertion_handler", []() -> py::object {
        if (argus::get_assertion_handler() != &python_assertion_thunk) {
            return py::none();
        }
        if (!python_assertion_handler()) {
            return py::none();
        }
        return python_assertion_handler();
    });
}
