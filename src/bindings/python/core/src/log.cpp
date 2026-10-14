#include <pch.hpp>

#include <pybind11/pybind11.h>

#include <argus/assert.hpp>
#include <argus/log.hpp>
#include <bind.hpp>

#include <string_view>

namespace py = pybind11;

namespace {

py::function& python_logger() {
    static py::function logger;
    return logger;
}

void python_log_thunk(argus::log::Level level,
                      std::string_view message) noexcept {
    if (!python_logger()) {
        return;
    }
    try {
        py::gil_scoped_acquire gil;
        python_logger()(level, py::str(message.data(), message.size()));
    } catch (...) {
    }
}

}  // namespace

void argus::python::bind_log(py::module_* module) {
    ARGUS_ASSERT(module != nullptr);

    py::enum_<argus::log::Level>(*module, "Level")
        .value("Trace", argus::log::Level::TRACE)
        .value("Debug", argus::log::Level::DEBUG)
        .value("Info", argus::log::Level::INFO)
        .value("Warn", argus::log::Level::WARN)
        .value("Error", argus::log::Level::ERROR)
        .value("Critical", argus::log::Level::CRITICAL);

    module->def("log", [](argus::log::Level level, std::string_view message) {
        argus::log::log(level, message);
    });
    module->def("trace",
                [](std::string_view message) { argus::log::trace(message); });
    module->def("debug",
                [](std::string_view message) { argus::log::debug(message); });
    module->def("info",
                [](std::string_view message) { argus::log::info(message); });
    module->def("warn",
                [](std::string_view message) { argus::log::warn(message); });
    module->def("error",
                [](std::string_view message) { argus::log::error(message); });
    module->def("critical", [](std::string_view message) {
        argus::log::critical(message);
    });

    module->def("set_logger", [](py::object callback) {
        if (callback.is_none()) {
            python_logger() = py::function();
            argus::log::reset_custom_logger();
            return;
        }
        python_logger() = py::function(callback);
        argus::log::set_custom_logger(&python_log_thunk);
    });
    module->def("reset_logger", []() {
        python_logger() = py::function();
        argus::log::reset_custom_logger();
    });
}
