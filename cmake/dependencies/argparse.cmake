# argparse dependency configuration
#
# This module handles finding argparse from multiple sources:
# 1. System packages (if available)
# 2. CPM download (fallback)
#
# Usage:
#   argus_require_dependency(argparse)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "argparse" OUTPUT_VAR _argparse_processed)
if(_argparse_processed)
  return()
endif()

argus_dep_header(NAME "argparse")

argus_dep_begin(
    NAME argparse
    VERSION ^3.0
    DEBIAN_NAMES libargparse-dev
    RPM_NAMES argparse-devel
    PACMAN_NAMES argparse
    BREW_NAMES argparse
    PKG_CONFIG_NAMES argparse
    CPM_NAME argparse
    CPM_VERSION 3.2
    CPM_GITHUB_REPOSITORY p-ranav/argparse
    CPM_GIT_TAG v3.2
)
argus_dep_end()

# Create argus::argparse::argparse alias
if(NOT TARGET argus::argparse::argparse)
  if(TARGET argparse::argparse)
    get_target_property(_argparse_aliased_target argparse::argparse ALIASED_TARGET)
    if(_argparse_aliased_target)
      add_library(argus::argparse::argparse ALIAS ${_argparse_aliased_target})
    else()
      add_library(argus::argparse::argparse ALIAS argparse::argparse)
    endif()
    argus_dep_log(SUCCESS "argparse configured (argparse::argparse)")
  elseif(TARGET argparse)
    add_library(argus::argparse::argparse ALIAS argparse)
    argus_dep_log(SUCCESS "argparse configured (argparse)")
  elseif(DEFINED argparse_SOURCE_DIR AND EXISTS "${argparse_SOURCE_DIR}/include/argparse/argparse.hpp")
    add_library(_argus_argparse_header_only INTERFACE)
    target_include_directories(_argus_argparse_header_only SYSTEM INTERFACE
            "${argparse_SOURCE_DIR}/include"
        )
    add_library(argus::argparse::argparse ALIAS _argus_argparse_header_only)
    argus_dep_log(SUCCESS "argparse configured (header-only)")
  else()
    argus_dep_log(NOT_FOUND "argparse")
  endif()
else()
  argus_dep_log(SUCCESS "argparse configured (argus::argparse::argparse)")
endif()

# Create argus::argparse convenience target
if(NOT TARGET _argus_argparse_all)
  add_library(_argus_argparse_all INTERFACE)
  if(TARGET argus::argparse::argparse)
    target_link_libraries(_argus_argparse_all INTERFACE argus::argparse::argparse)
  endif()
endif()

if(NOT TARGET argus::argparse)
  add_library(argus::argparse ALIAS _argus_argparse_all)
endif()
