# concurrentqueue dependency configuration
#
# This module handles finding concurrentqueue from multiple sources:
# 1. System packages (pacman, apt, etc.)
# 2. CPM download (fallback)
#
# Usage:
#   argus_require_dependency(concurrentqueue)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "concurrentqueue" OUTPUT_VAR _concurrentqueue_processed)
if(_concurrentqueue_processed)
  return()
endif()

argus_dep_header(NAME "concurrentqueue")

# Use argus_module system for standard package finding
argus_dep_begin(
    NAME concurrentqueue
    VERSION ~1.0
    DEBIAN_NAMES concurrentqueue-dev
    BREW_NAMES concurrentqueue
    PKG_CONFIG_NAMES concurrentqueue
    CPM_NAME concurrentqueue
    CPM_VERSION 1.0.4
    CPM_GITHUB_REPOSITORY cameron314/concurrentqueue
)
argus_dep_end()

# Create argus::concurrentqueue::concurrentqueue alias if concurrentqueue was found
if(NOT TARGET argus::concurrentqueue::concurrentqueue)
  if(TARGET concurrentqueue::concurrentqueue)
    add_library(argus::concurrentqueue::concurrentqueue ALIAS concurrentqueue::concurrentqueue)
    argus_dep_log(SUCCESS "concurrentqueue configured (concurrentqueue::concurrentqueue)")
  elseif(TARGET concurrentqueue)
    if(NOT TARGET concurrentqueue::concurrentqueue)
      add_library(concurrentqueue::concurrentqueue ALIAS concurrentqueue)
    endif()
    add_library(argus::concurrentqueue::concurrentqueue ALIAS concurrentqueue)
    argus_dep_log(SUCCESS "concurrentqueue configured (concurrentqueue)")
  else()
    argus_dep_log(NOT_FOUND "concurrentqueue")
  endif()
else()
  argus_dep_log(SUCCESS "concurrentqueue configured (argus::concurrentqueue::concurrentqueue)")
endif()

# Create argus::concurrentqueue convenience target that brings in all concurrentqueue targets
if(NOT TARGET _argus_concurrentqueue_all)
  add_library(_argus_concurrentqueue_all INTERFACE)
  if(TARGET argus::concurrentqueue::concurrentqueue)
    target_link_libraries(_argus_concurrentqueue_all INTERFACE argus::concurrentqueue::concurrentqueue)
  endif()
endif()

if(NOT TARGET argus::concurrentqueue)
  add_library(argus::concurrentqueue ALIAS _argus_concurrentqueue_all)
endif()
