set(MODULE_NAME "BINDINGS_PYTHON")

set(ARGUS_BINDINGS_PYTHON_HEADERS
)

set(ARGUS_BINDINGS_PYTHON_SOURCES
    src/module.cpp
)

set(ARGUS_BINDINGS_PYTHON_ALL_FILES
    ${ARGUS_BINDINGS_PYTHON_HEADERS}
    ${ARGUS_BINDINGS_PYTHON_SOURCES}
)

add_library(argus_bindings_python_lib STATIC ${ARGUS_BINDINGS_PYTHON_ALL_FILES})
add_library(argus::bindings::python_lib ALIAS argus_bindings_python_lib)

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_bindings_python_lib STANDARD 23)
argus_target_set_platform(argus_bindings_python_lib)
argus_target_set_output_dirs(argus_bindings_python_lib MODULE bindings/python)
argus_target_set_folder(argus_bindings_python_lib "argus/bindings/python")

argus_target_add_pch(argus_bindings_python_lib ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
argus_target_set_warnings(argus_bindings_python_lib)
argus_target_set_optimization(argus_bindings_python_lib)
argus_target_enable_lto(argus_bindings_python_lib)

include(${PROJECT_SOURCE_DIR}/cmake/Dependencies.cmake)
argus_require_dependency(pybind11)

target_include_directories(argus_bindings_python_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(argus_bindings_python_lib PRIVATE
    argus::core
    argus::common
    argus::algo
    argus::pybind11::pybind11
)

pybind11_add_module(argus_bindings_python MODULE
    ${ARGUS_BINDINGS_PYTHON_ALL_FILES}
)

argus_target_set_cxx_standard(argus_bindings_python STANDARD 23)
argus_target_set_platform(argus_bindings_python)
argus_target_set_output_dirs(argus_bindings_python MODULE bindings/python)
argus_target_set_folder(argus_bindings_python "argus/bindings/python")

argus_target_add_pch(argus_bindings_python ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
argus_target_set_warnings(argus_bindings_python)
argus_target_set_optimization(argus_bindings_python)
# argus_target_enable_lto(argus_bindings_python) # Could cause issues with pybind11

set_target_properties(argus_bindings_python PROPERTIES
    OUTPUT_NAME "argus_bindings"
    PREFIX ""
)

target_include_directories(argus_bindings_python
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(argus_bindings_python PRIVATE
    argus::core
    argus::algo
    argus::pybind11::pybind11
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# Install path option
set(ARGUS_PYTHON_INSTALL_DIR "" CACHE PATH
    "Installation directory for Python bindings")

# Installation
if(ARGUS_PYTHON_INSTALL_DIR)
  install(TARGETS argus_bindings_python
      LIBRARY DESTINATION ${ARGUS_PYTHON_INSTALL_DIR}
  )
else()
  install(TARGETS argus_bindings_python
      LIBRARY DESTINATION lib
  )
endif()
