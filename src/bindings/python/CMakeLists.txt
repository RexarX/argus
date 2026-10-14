set(_argus_python_sources
    argus/src/module.cpp
    argus/src/pch.hpp
    core/src/log.cpp
    core/src/assert.cpp
)

set(_argus_python_libs argus::core)

if(ARGUS_BUILD_ALGO)
  list(APPEND _argus_python_sources algo/src/module.cpp)
  list(APPEND _argus_python_libs argus::algo)
endif()

if(ARGUS_BUILD_SIM)
  list(APPEND _argus_python_sources sim/src/module.cpp)
  list(APPEND _argus_python_libs argus::sim)
endif()

pybind11_add_module(argus_python ${_argus_python_sources})
target_link_libraries(argus_python PRIVATE ${_argus_python_libs})

# `argus.<abi>.pyd`. The ABI suffix is the CPython tag (for example
# `.cp314-win_amd64`), not the argus version. Submodules are registered on
# this module, so `import argus.algo` does not need a package directory.
# Embree's shared libraries are copied beside this file.
set_target_properties(argus_python PROPERTIES OUTPUT_NAME "argus")

argus_apply_conventions(argus_python ALLOW_EXCEPTIONS)
argus_target_set_output_dirs(argus_python CUSTOM_FOLDER python)
argus_target_set_folder(argus_python "bindings/python")
argus_target_add_pch(argus_python
    "${CMAKE_CURRENT_SOURCE_DIR}/argus/src/pch.hpp")

if(MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
    AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
  target_compile_options(argus_python PRIVATE /EHsc /GR)
  target_compile_definitions(argus_python PRIVATE _HAS_EXCEPTIONS=1)
else()
  target_compile_options(argus_python PRIVATE -fexceptions -frtti)
endif()

target_include_directories(argus_python PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/argus/src
)

if(ARGUS_ENABLE_EMBREE_BACKEND AND TARGET argus_sim)
  argus_target_link_embree(argus_python)
  argus_target_copy_embree_runtime(argus_python)
endif()
