# spdlog dependency configuration
#
# This module handles finding spdlog from multiple sources:
# 1. System packages (pacman, apt, etc.)
# 2. CPM download (fallback)
#
# Usage in plugin CMakeLists.txt:
#   argus_require_dependency(spdlog)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "spdlog" OUTPUT_VAR _spdlog_processed)
if(_spdlog_processed)
  return()
endif()

argus_dep_header(NAME "spdlog")

# Use argus_module system for standard package finding
argus_dep_begin(
    NAME spdlog
    VERSION ^1.12
    DEBIAN_NAMES libspdlog-dev
    RPM_NAMES spdlog-devel
    PACMAN_NAMES spdlog
    BREW_NAMES spdlog
    PKG_CONFIG_NAMES spdlog
    CPM_NAME spdlog
    CPM_VERSION 1.17.0
    CPM_GITHUB_REPOSITORY gabime/spdlog
    CPM_OPTIONS
        "SPDLOG_BUILD_SHARED OFF"
        "SPDLOG_BUILD_EXAMPLE OFF"
        "SPDLOG_BUILD_TESTS OFF"
        "SPDLOG_USE_STD_FORMAT ON"
        "SPDLOG_FMT_EXTERNAL OFF"
)

argus_dep_end()

# Helper function to get the real (non-alias) target from a potentially aliased target
function(_argus_spdlog_get_real_target target_name out_var)
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

# Create argus::spdlog::spdlog alias target for spdlog
if(TARGET spdlog::spdlog)
  if(NOT TARGET argus::spdlog::spdlog)
    _argus_spdlog_get_real_target(spdlog::spdlog _spdlog_real)
    if(_spdlog_real)
      add_library(argus::spdlog::spdlog ALIAS ${_spdlog_real})
    else()
      add_library(argus::spdlog::spdlog ALIAS spdlog::spdlog)
    endif()
  endif()
  argus_dep_log(SUCCESS "spdlog configured (compiled)")
endif()

if(TARGET spdlog::spdlog_header_only)
  if(NOT TARGET argus::spdlog::spdlog_header_only)
    _argus_spdlog_get_real_target(spdlog::spdlog_header_only _spdlog_header_real)
    if(_spdlog_header_real)
      add_library(argus::spdlog::spdlog_header_only ALIAS ${_spdlog_header_real})
    else()
      add_library(argus::spdlog::spdlog_header_only ALIAS spdlog::spdlog_header_only)
    endif()
  endif()
  argus_dep_log(SUCCESS "spdlog header-only configured")
endif()

# Fallbacks for older or non-namespaced targets
if(TARGET spdlog AND NOT TARGET argus::spdlog::spdlog)
  _argus_spdlog_get_real_target(spdlog _spdlog_real)
  if(_spdlog_real)
    add_library(argus::spdlog::spdlog ALIAS ${_spdlog_real})
  else()
    add_library(argus::spdlog::spdlog ALIAS spdlog)
  endif()
  argus_dep_log(SUCCESS "spdlog configured (fallback)")
endif()

if(TARGET spdlog_header_only AND NOT TARGET argus::spdlog::spdlog_header_only)
  _argus_spdlog_get_real_target(spdlog_header_only _spdlog_header_real)
  if(_spdlog_header_real)
    add_library(argus::spdlog::spdlog_header_only ALIAS ${_spdlog_header_real})
  else()
    add_library(argus::spdlog::spdlog_header_only ALIAS spdlog_header_only)
  endif()
  argus_dep_log(SUCCESS "spdlog header-only configured (fallback)")
endif()

# If we have argus::spdlog::spdlog but not argus::spdlog::spdlog_header_only, create the header-only alias
# This handles cases where pkg-config only provides the compiled library target
if(TARGET argus::spdlog::spdlog AND NOT TARGET argus::spdlog::spdlog_header_only)
  # Get the real target that argus::spdlog::spdlog points to
  _argus_spdlog_get_real_target(argus::spdlog::spdlog _spdlog_real)
  if(_spdlog_real)
    add_library(argus::spdlog::spdlog_header_only ALIAS ${_spdlog_real})
  else()
    # If we can't get the real target, create an interface library instead
    add_library(_argus_spdlog_header_only_compat INTERFACE)
    target_link_libraries(_argus_spdlog_header_only_compat INTERFACE argus::spdlog::spdlog)
    add_library(argus::spdlog::spdlog_header_only ALIAS _argus_spdlog_header_only_compat)
  endif()
  argus_dep_log(STATUS "spdlog header-only using compiled library")
endif()

# Create argus::spdlog convenience target that brings in all spdlog targets
if(NOT TARGET _argus_spdlog_all)
  add_library(_argus_spdlog_all INTERFACE)
  if(TARGET argus::spdlog::spdlog)
    target_link_libraries(_argus_spdlog_all INTERFACE argus::spdlog::spdlog)
  endif()
endif()

if(NOT TARGET argus::spdlog)
  add_library(argus::spdlog ALIAS _argus_spdlog_all)
endif()

# Warn if neither target is found
if(NOT TARGET argus::spdlog::spdlog AND NOT TARGET argus::spdlog::spdlog_header_only)
  argus_dep_log(NOT_FOUND "spdlog")
endif()
