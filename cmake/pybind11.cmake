# Provide pybind11 for the Python bindings.
#
# Resolution, only when ARGUS_BUILD_PYTHON_BINDINGS is ON:
#   1. find_package(pybind11) 3.x, at least ARGUS_PYBIND11_VERSION
#   2. CPM fetch of that version from GitHub
#
# 4.x is rejected so the bindings stay on the 3.x line.

include_guard(GLOBAL)
include(CPM)

set(ARGUS_PYBIND11_VERSION "3.1.0" CACHE STRING
    "Minimum pybind11 3.x version. CPM fetches this tag when find_package misses."
)

function(_argus_pybind11_version_ok OUT_VAR)
  if(NOT pybind11_FOUND OR NOT pybind11_VERSION)
    set(${OUT_VAR} FALSE PARENT_SCOPE)
    return()
  endif()
  if(pybind11_VERSION VERSION_LESS "${ARGUS_PYBIND11_VERSION}")
    set(${OUT_VAR} FALSE PARENT_SCOPE)
    return()
  endif()
  if(pybind11_VERSION VERSION_GREATER_EQUAL "4.0.0")
    set(${OUT_VAR} FALSE PARENT_SCOPE)
    return()
  endif()
  set(${OUT_VAR} TRUE PARENT_SCOPE)
endfunction()

function(_argus_register_pybind11_lib)
  if(TARGET pybind11::module)
    argus_register_lib_target(pybind11 pybind11::module)
  elseif(TARGET pybind11::headers)
    argus_register_lib_target(pybind11 pybind11::headers)
  endif()
endfunction()

#[[
    argus_setup_pybind11()

    Ensures pybind11::headers / pybind11_add_module are available.
]]
function(argus_setup_pybind11)
  if(TARGET pybind11::module OR TARGET pybind11::headers)
    _argus_pybind11_version_ok(_ok)
    if(_ok OR NOT pybind11_VERSION)
      if(pybind11_VERSION)
        message(STATUS "pybind11: using existing target ${pybind11_VERSION}")
      else()
        message(STATUS "pybind11: using existing target")
      endif()
      _argus_register_pybind11_lib()
      return()
    endif()
    message(FATAL_ERROR
        "pybind11 ${pybind11_VERSION} is already defined, but argus requires "
        "${ARGUS_PYBIND11_VERSION} <= version < 4.0.0."
    )
  endif()

  find_package(pybind11 ${ARGUS_PYBIND11_VERSION} CONFIG QUIET)
  _argus_pybind11_version_ok(_ok)
  if(_ok)
    message(STATUS "pybind11: found ${pybind11_VERSION} via find_package")
    _argus_register_pybind11_lib()
    return()
  endif()

  if(pybind11_FOUND AND pybind11_VERSION VERSION_GREATER_EQUAL "4.0.0")
    message(FATAL_ERROR
        "pybind11 ${pybind11_VERSION} was found, but argus requires 3.x "
        "(>= ${ARGUS_PYBIND11_VERSION}). Remove it from CMAKE_PREFIX_PATH or "
        "install a 3.x package."
    )
  endif()

  if(pybind11_FOUND)
    message(STATUS
        "pybind11: found ${pybind11_VERSION}, older than ${ARGUS_PYBIND11_VERSION}; fetching"
    )
  else()
    message(STATUS "pybind11: fetching v${ARGUS_PYBIND11_VERSION} via CPM")
  endif()

  CPMAddPackage(
      NAME pybind11
      GITHUB_REPOSITORY pybind/pybind11
      GIT_TAG "v${ARGUS_PYBIND11_VERSION}"
      VERSION "${ARGUS_PYBIND11_VERSION}"
      GIT_SHALLOW TRUE
      EXCLUDE_FROM_ALL YES
      OPTIONS
        "PYBIND11_TEST OFF"
        "PYBIND11_INSTALL OFF"
  )

  if(NOT TARGET pybind11::module AND NOT TARGET pybind11::headers)
    message(FATAL_ERROR "pybind11: CPM fetch did not provide pybind11")
  endif()

  _argus_register_pybind11_lib()
endfunction()
