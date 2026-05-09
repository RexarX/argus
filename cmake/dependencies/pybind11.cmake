# pybind11 dependency configuration
#
# This module handles finding pybind11 from multiple sources:
# 1. System packages (pacman, apt, etc.)
# 2. CPM download (fallback)
#
# Usage:
#   argus_require_dependency(pybind11)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "pybind11" OUTPUT_VAR _pybind11_processed)
if(_pybind11_processed)
  return()
endif()

argus_dep_header(NAME "pybind11")

# Use argus_module system for standard package finding
argus_dep_begin(
    NAME pybind11
    VERSION ^3.0.0
    DEBIAN_NAMES pybind11-dev
    RPM_NAMES pybind11-devel
    PACMAN_NAMES pybind11
    BREW_NAMES pybind11
    PKG_CONFIG_NAMES pybind11
    CPM_NAME pybind11
    CPM_VERSION 3.0.4
    CPM_GITHUB_REPOSITORY pybind/pybind11
    CPM_GIT_TAG v3.0.4
    CPM_OPTIONS
        "PYBIND11_INSTALL ON"
        "PYBIND11_TEST OFF"
        "PYBIND11_NOPYTHON OFF"
)
argus_dep_end()

# Helper function to resolve alias targets
function(_argus_pybind_get_real_target target_name out_var)
  if(NOT TARGET ${target_name})
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()

  set(_current_target ${target_name})
  set(_max_iterations 10)
  set(_iteration 0)

  while(_iteration LESS _max_iterations)
    get_target_property(_aliased ${_current_target} ALIASED_TARGET)
    if(_aliased)
      set(_current_target ${_aliased})
    else()
      break()
    endif()
    math(EXPR _iteration "${_iteration} + 1")
  endwhile()

  set(${out_var} ${_current_target} PARENT_SCOPE)
endfunction()

# pkg-config fallback: create pybind11::pybind11 from the generated target
if(NOT TARGET pybind11::pybind11 AND NOT TARGET pybind11::headers)
  if(TARGET pybind11)
    add_library(pybind11::pybind11 ALIAS pybind11)
  endif()
endif()

# Create argus::pybind11::pybind11 alias for the main pybind11 target
if(TARGET pybind11::pybind11)
  if(NOT TARGET argus::pybind11::pybind11)
    _argus_pybind_get_real_target(pybind11::pybind11 _pybind11_real)
    if(_pybind11_real)
      add_library(argus::pybind11::pybind11 ALIAS ${_pybind11_real})
    else()
      add_library(argus::pybind11::pybind11 ALIAS pybind11::pybind11)
    endif()
  endif()
  argus_dep_log(SUCCESS "pybind11 configured (pybind11::pybind11)")
endif()

# Create argus::pybind11::headers alias for the header-only target
if(TARGET pybind11::headers)
  if(NOT TARGET argus::pybind11::headers)
    _argus_pybind_get_real_target(pybind11::headers _pybind11_headers_real)
    if(_pybind11_headers_real)
      add_library(argus::pybind11::headers ALIAS ${_pybind11_headers_real})
    else()
      add_library(argus::pybind11::headers ALIAS pybind11::headers)
    endif()
  endif()
  argus_dep_log(SUCCESS "pybind11 headers configured")
endif()

# pkg-config fallback: create argus::pybind11::pybind11 from the pybind11 target
if(NOT TARGET argus::pybind11::pybind11 AND NOT TARGET argus::pybind11::headers)
  if(TARGET pybind11)
    add_library(argus::pybind11::pybind11 ALIAS pybind11)
    argus_dep_log(SUCCESS "pybind11 configured (via pkg-config)")
  endif()
endif()

# Create argus::pybind11::module alias for the module target
if(TARGET pybind11::module)
  if(NOT TARGET argus::pybind11::module)
    _argus_pybind_get_real_target(pybind11::module _pybind11_module_real)
    if(_pybind11_module_real)
      add_library(argus::pybind11::module ALIAS ${_pybind11_module_real})
    else()
      add_library(argus::pybind11::module ALIAS pybind11::module)
    endif()
  endif()
  argus_dep_log(SUCCESS "pybind11 module configured")
endif()

# Create argus::pybind11::embed alias for the embed target
if(TARGET pybind11::embed)
  if(NOT TARGET argus::pybind11::embed)
    _argus_pybind_get_real_target(pybind11::embed _pybind11_embed_real)
    if(_pybind11_embed_real)
      add_library(argus::pybind11::embed ALIAS ${_pybind11_embed_real})
    else()
      add_library(argus::pybind11::embed ALIAS pybind11::embed)
    endif()
  endif()
  argus_dep_log(SUCCESS "pybind11 embed configured")
endif()

# Create argus::pybind11 convenience target that brings in all pybind11 targets
if(NOT TARGET _argus_pybind11_all)
  add_library(_argus_pybind11_all INTERFACE)
  if(TARGET argus::pybind11::pybind11)
    target_link_libraries(_argus_pybind11_all INTERFACE argus::pybind11::pybind11)
  endif()
  if(TARGET argus::pybind11::headers)
    target_link_libraries(_argus_pybind11_all INTERFACE argus::pybind11::headers)
  endif()
endif()

if(NOT TARGET argus::pybind11)
  add_library(argus::pybind11 ALIAS _argus_pybind11_all)
endif()

# Warn if main targets are not found
if(NOT TARGET argus::pybind11::pybind11 AND NOT TARGET argus::pybind11::headers AND NOT TARGET pybind11)
  argus_dep_log(NOT_FOUND "pybind11")
endif()
