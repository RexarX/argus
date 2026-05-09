# eigen dependency configuration
#
# This module handles finding Eigen from multiple sources:
# 1. System packages (pacman, apt, etc.)
# 2. CPM download (fallback)
#
# Usage:
#   argus_require_dependency(eigen)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "eigen" OUTPUT_VAR _eigen_processed)
if(_eigen_processed)
  return()
endif()

argus_dep_header(NAME "eigen")

# Use argus_module system for standard package finding
argus_dep_begin(
    NAME eigen
    VERSION ^5.0
    DEBIAN_NAMES libeigen3-dev
    RPM_NAMES eigen3-devel
    PACMAN_NAMES eigen
    BREW_NAMES eigen
    PKG_CONFIG_NAMES eigen3
    CPM_NAME Eigen
    CPM_VERSION 5.0.1
    CPM_GITLAB_REPOSITORY libeigen/eigen
    CPM_GIT_TAG 5.0.1
    CPM_OPTIONS
        "EIGEN_BUILD_DOC OFF"
        "EIGEN_BUILD_PKGCONFIG ON"
        "BUILD_TESTING OFF"
        "EIGEN_BUILD_BLAS OFF"
        "EIGEN_BUILD_LAPACK OFF"
)
argus_dep_end()

# Helper function to resolve alias targets
function(_argus_eigen_get_real_target target_name out_var)
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

# Create argus::eigen::eigen alias target
if(TARGET Eigen3::Eigen)
  if(NOT TARGET argus::eigen::eigen)
    _argus_eigen_get_real_target(Eigen3::Eigen _eigen_real)
    if(_eigen_real)
      add_library(argus::eigen::eigen ALIAS ${_eigen_real})
    else()
      add_library(argus::eigen::eigen ALIAS Eigen3::Eigen)
    endif()
  endif()
  argus_dep_log(SUCCESS "eigen configured (Eigen3::Eigen)")
endif()

# Fallback: some package managers create eigen without full namespacing
if(TARGET eigen AND NOT TARGET argus::eigen::eigen AND NOT TARGET Eigen3::Eigen)
  _argus_eigen_get_real_target(eigen _eigen_real)
  if(_eigen_real)
    add_library(argus::eigen::eigen ALIAS ${_eigen_real})
  else()
    add_library(argus::eigen::eigen ALIAS eigen)
  endif()
  argus_dep_log(SUCCESS "eigen configured (eigen - fallback)")
endif()

# pkg-config fallback: create Eigen3::Eigen from pkg_check_modules target
if(NOT TARGET argus::eigen::eigen AND NOT TARGET Eigen3::Eigen)
  if(TARGET PkgConfig::Eigen3)
    add_library(argus::eigen::eigen ALIAS PkgConfig::Eigen3)
    argus_dep_log(SUCCESS "eigen configured (via pkg-config)")
  endif()
endif()

# Create argus::eigen convenience target that brings in all eigen targets
if(NOT TARGET _argus_eigen_all)
  add_library(_argus_eigen_all INTERFACE)
  if(TARGET argus::eigen::eigen)
    target_link_libraries(_argus_eigen_all INTERFACE argus::eigen::eigen)
  endif()
endif()

if(NOT TARGET argus::eigen)
  add_library(argus::eigen ALIAS _argus_eigen_all)
endif()

# Warn if no target was found
if(NOT TARGET argus::eigen::eigen)
  argus_dep_log(NOT_FOUND "eigen")
endif()
