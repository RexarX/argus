#pragma once

#include <pybind11/pybind11.h>

namespace argus::python {

void bind_log(pybind11::module_* module);
void bind_assert(pybind11::module_* module);
void bind_algo(pybind11::module_* module);
void bind_sim(pybind11::module_* module);

}  // namespace argus::python
