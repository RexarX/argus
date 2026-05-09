# AdaptiveCpp dependency configuration
#
# This module handles finding a system-installed AdaptiveCpp compiler and runtime.
# AdaptiveCpp is a complete heterogeneous programming platform with heavy dependencies
# (LLVM, GPU drivers/runtimes like CUDA or ROCm) that cannot be built via CPM.
#
# Usage:
#   argus_require_dependency(AdaptiveCpp)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "AdaptiveCpp" OUTPUT_VAR _adaptivecpp_processed)
if(_adaptivecpp_processed)
  return()
endif()

argus_dep_header(NAME "AdaptiveCpp")

# Use argus_module system for standard package finding
# Note: No CPM_* arguments are provided, as AdaptiveCpp requires a system install.
argus_dep_begin(
    NAME AdaptiveCpp
    VERSION ^25.10
    DEBIAN_NAMES adaptivecpp-dev
    RPM_NAMES adaptivecpp-devel
    PACMAN_NAMES adaptivecpp
    BREW_NAMES adaptivecpp
    PKG_CONFIG_NAMES adaptivecpp
)
argus_dep_end()

# Helper function to resolve alias targets
function(_argus_adaptivecpp_get_real_target target_name out_var)
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

# Handle AdaptiveCpp target - it's typically found via find_package
# AdaptiveCpp usually creates targets like:
# - AdaptiveCpp::AdaptiveCpp (main target)
# - AdaptiveCpp::acpp (compiler target if needed)

# Check for the namespaced target first
if(TARGET AdaptiveCpp::AdaptiveCpp)
  if(NOT TARGET argus::AdaptiveCpp::AdaptiveCpp)
    _argus_adaptivecpp_get_real_target(AdaptiveCpp::AdaptiveCpp _adaptivecpp_real)
    if(_adaptivecpp_real)
      add_library(argus::AdaptiveCpp::AdaptiveCpp ALIAS ${_adaptivecpp_real})
    else()
      add_library(argus::AdaptiveCpp::AdaptiveCpp ALIAS AdaptiveCpp::AdaptiveCpp)
    endif()
  endif()
  argus_dep_log(SUCCESS "AdaptiveCpp configured (AdaptiveCpp::AdaptiveCpp)")
endif()

# Check for AdaptiveCpp::acpp-common and AdaptiveCpp::acpp-rt (standard exported targets)
if((TARGET AdaptiveCpp::acpp-common OR TARGET AdaptiveCpp::acpp-rt) AND NOT TARGET argus::AdaptiveCpp::AdaptiveCpp)
  if(NOT TARGET _argus_adaptivecpp_runtime)
    add_library(_argus_adaptivecpp_runtime INTERFACE)
    if(TARGET AdaptiveCpp::acpp-common)
      target_link_libraries(_argus_adaptivecpp_runtime INTERFACE AdaptiveCpp::acpp-common)
    endif()
    if(TARGET AdaptiveCpp::acpp-rt)
      target_link_libraries(_argus_adaptivecpp_runtime INTERFACE AdaptiveCpp::acpp-rt)
    endif()
  endif()
  add_library(argus::AdaptiveCpp::AdaptiveCpp ALIAS _argus_adaptivecpp_runtime)
  argus_dep_log(SUCCESS "AdaptiveCpp configured (acpp-common / acpp-rt)")
endif()

# Check for the old hipSYCL namespaced target (backward compatibility)
if(TARGET hipsycl::hipsycl AND NOT TARGET argus::AdaptiveCpp::AdaptiveCpp)
  if(NOT TARGET argus::AdaptiveCpp::AdaptiveCpp)
    _argus_adaptivecpp_get_real_target(hipsycl::hipsycl _adaptivecpp_real)
    if(_adaptivecpp_real)
      add_library(argus::AdaptiveCpp::AdaptiveCpp ALIAS ${_adaptivecpp_real})
    else()
      add_library(argus::AdaptiveCpp::AdaptiveCpp ALIAS hipsycl::hipsycl)
    endif()
  endif()
  argus_dep_log(SUCCESS "AdaptiveCpp configured (via hipsycl::hipsycl)")
endif()

# Check for non-namespaced targets
if(TARGET adaptivecpp AND NOT TARGET argus::AdaptiveCpp::AdaptiveCpp)
  _argus_adaptivecpp_get_real_target(adaptivecpp _adaptivecpp_real)
  if(_adaptivecpp_real)
    add_library(argus::AdaptiveCpp::AdaptiveCpp ALIAS ${_adaptivecpp_real})
  else()
    add_library(argus::AdaptiveCpp::AdaptiveCpp ALIAS adaptivecpp)
  endif()
  argus_dep_log(SUCCESS "AdaptiveCpp configured (adaptivecpp)")
endif()

# Check for old hipsycl target
if(TARGET hipsycl AND NOT TARGET argus::AdaptiveCpp::AdaptiveCpp)
  _argus_adaptivecpp_get_real_target(hipsycl _adaptivecpp_real)
  if(_adaptivecpp_real)
    add_library(argus::AdaptiveCpp::AdaptiveCpp ALIAS ${_adaptivecpp_real})
  else()
    add_library(argus::AdaptiveCpp::AdaptiveCpp ALIAS hipsycl)
  endif()
  argus_dep_log(SUCCESS "AdaptiveCpp configured (hipsycl)")
endif()

# Check if the compiler was found (acpp or syclcc)
find_program(_ADAPTIVECPP_COMPILER
    NAMES acpp syclcc
    PATHS
        ${AdaptiveCpp_ROOT}
        ${ADAPTIVECPP_ROOT}
        $ENV{AdaptiveCpp_ROOT}
        $ENV{ADAPTIVECPP_ROOT}
    PATH_SUFFIXES bin
)

if(_ADAPTIVECPP_COMPILER)
  argus_dep_log(STATUS "AdaptiveCpp compiler found: ${_ADAPTIVECPP_COMPILER}")
endif()

# Create argus::AdaptiveCpp convenience target that brings in all AdaptiveCpp targets
if(NOT TARGET _argus_adaptivecpp_all)
  add_library(_argus_adaptivecpp_all INTERFACE)
  if(TARGET argus::AdaptiveCpp::AdaptiveCpp)
    target_link_libraries(_argus_adaptivecpp_all INTERFACE argus::AdaptiveCpp::AdaptiveCpp)
  endif()
endif()

if(NOT TARGET argus::AdaptiveCpp)
  add_library(argus::AdaptiveCpp ALIAS _argus_adaptivecpp_all)
endif()

# Warn if no targets were found
if(NOT TARGET argus::AdaptiveCpp::AdaptiveCpp)
  argus_dep_log(NOT_FOUND "AdaptiveCpp - no library target found")
endif()
