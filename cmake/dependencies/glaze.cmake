# glaze dependency configuration
#
# This module handles finding glaze from multiple sources:
# 1. System packages (if available)
# 2. CPM download (fallback)
#
# Usage:
#   argus_require_dependency(glaze)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "glaze" OUTPUT_VAR _glaze_processed)
if(_glaze_processed)
  return()
endif()

argus_dep_header(NAME "glaze")

argus_dep_begin(
    NAME glaze
    VERSION ^7.0
    DEBIAN_NAMES glaze-dev
    PACMAN_NAMES glaze
    BREW_NAMES glaze
    PKG_CONFIG_NAMES glaze
    CPM_NAME glaze
    CPM_VERSION 7.5.0
    CPM_GITHUB_REPOSITORY stephenberry/glaze
    CPM_GIT_TAG v7.5.0
)
argus_dep_end()

# Create argus::glaze::glaze alias
if(NOT TARGET argus::glaze::glaze)
  if(TARGET glaze::glaze)
    get_target_property(_glaze_aliased_target glaze::glaze ALIASED_TARGET)
    if(_glaze_aliased_target)
      add_library(argus::glaze::glaze ALIAS ${_glaze_aliased_target})
    else()
      add_library(argus::glaze::glaze ALIAS glaze::glaze)
    endif()
    argus_dep_log(SUCCESS "glaze configured (glaze::glaze)")
  elseif(TARGET glaze)
    add_library(argus::glaze::glaze ALIAS glaze)
    argus_dep_log(SUCCESS "glaze configured (glaze)")
  elseif(DEFINED glaze_SOURCE_DIR AND EXISTS "${glaze_SOURCE_DIR}/include/glaze/glaze.hpp")
    add_library(_argus_glaze_header_only INTERFACE)
    target_include_directories(_argus_glaze_header_only SYSTEM INTERFACE
            "${glaze_SOURCE_DIR}/include"
        )
    add_library(argus::glaze::glaze ALIAS _argus_glaze_header_only)
    argus_dep_log(SUCCESS "glaze configured (header-only)")
  else()
    argus_dep_log(NOT_FOUND "glaze")
  endif()
else()
  argus_dep_log(SUCCESS "glaze configured (argus::glaze::glaze)")
endif()

# Create argus::glaze convenience target
if(NOT TARGET _argus_glaze_all)
  add_library(_argus_glaze_all INTERFACE)
  if(TARGET argus::glaze::glaze)
    target_link_libraries(_argus_glaze_all INTERFACE argus::glaze::glaze)
  endif()
endif()

if(NOT TARGET argus::glaze)
  add_library(argus::glaze ALIAS _argus_glaze_all)
endif()
