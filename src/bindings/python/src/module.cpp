#include <pch.hpp>

#include <pybind11/pybind11.h>

PYBIND11_MODULE(argus_bindings, module) {
  module.doc() = "Argus bindings for Python";
}
