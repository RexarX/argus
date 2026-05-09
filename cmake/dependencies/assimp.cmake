# assimp dependency configuration
#
# This module handles finding assimp from multiple sources:
# 1. System packages (pacman, apt, etc.)
# 2. CPM download (fallback)
#
# Usage:
#   argus_require_dependency(assimp)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "assimp" OUTPUT_VAR _assimp_processed)
if(_assimp_processed)
  return()
endif()

argus_dep_header(NAME "assimp")

# Use argus_module system for standard package finding
argus_dep_begin(
    NAME assimp
    VERSION ^6.0
    DEBIAN_NAMES libassimp-dev
    RPM_NAMES assimp-devel
    PACMAN_NAMES assimp
    BREW_NAMES assimp
    PKG_CONFIG_NAMES assimp
    CPM_NAME assimp
    CPM_VERSION 6.0.5
    CPM_GITHUB_REPOSITORY assimp/assimp
    CPM_GIT_TAG v6.0.5
    CPM_OPTIONS
        "ASSIMP_BUILD_TESTS OFF"
        "ASSIMP_BUILD_ASSIMP_TOOLS OFF"
        "ASSIMP_BUILD_SAMPLES OFF"
        "ASSIMP_BUILD_DOCS OFF"
        "ASSIMP_INSTALL ON"
        "ASSIMP_WARNINGS_AS_ERRORS OFF"
        "ASSIMP_INJECT_DEBUG_POSTFIX OFF"
)
argus_dep_end()

# Helper function to resolve alias targets
function(_argus_assimp_get_real_target target_name out_var)
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

# Create argus::assimp::assimp alias target
if(TARGET assimp::assimp)
  if(NOT TARGET argus::assimp::assimp)
    _argus_assimp_get_real_target(assimp::assimp _assimp_real)
    if(_assimp_real)
      add_library(argus::assimp::assimp ALIAS ${_assimp_real})
    else()
      add_library(argus::assimp::assimp ALIAS assimp::assimp)
    endif()
  endif()
  argus_dep_log(SUCCESS "assimp configured (assimp::assimp)")
endif()

# Fallback for non-namespaced target (older CMake configs or vcpkg)
if(TARGET assimp AND NOT TARGET argus::assimp::assimp)
  _argus_assimp_get_real_target(assimp _assimp_real)
  if(_assimp_real)
    add_library(argus::assimp::assimp ALIAS ${_assimp_real})
  else()
    add_library(argus::assimp::assimp ALIAS assimp)
  endif()
  argus_dep_log(SUCCESS "assimp configured (assimp - fallback)")
endif()

# Create argus::assimp convenience target that brings in all assimp targets
if(NOT TARGET _argus_assimp_all)
  add_library(_argus_assimp_all INTERFACE)
  if(TARGET argus::assimp::assimp)
    target_link_libraries(_argus_assimp_all INTERFACE argus::assimp::assimp)
  endif()
endif()

if(NOT TARGET argus::assimp)
  add_library(argus::assimp ALIAS _argus_assimp_all)
endif()

# Warn if no target was found
if(NOT TARGET argus::assimp::assimp)
  argus_dep_log(NOT_FOUND "assimp")
endif()
