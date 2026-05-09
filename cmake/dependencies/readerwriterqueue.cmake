# readerwriterqueue dependency configuration
#
# This module handles finding readerwriterqueue from multiple sources:
# 1. System packages (if available)
# 2. CPM download (fallback)
#
# Usage:
#   argus_require_dependency(readerwriterqueue)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "readerwriterqueue" OUTPUT_VAR _readerwriterqueue_processed)
if(_readerwriterqueue_processed)
  return()
endif()

argus_dep_header(NAME "readerwriterqueue")

argus_dep_begin(
    NAME readerwriterqueue
    VERSION ~1.0.0
    DEBIAN_NAMES readerwriterqueue-dev
    BREW_NAMES readerwriterqueue
    PKG_CONFIG_NAMES readerwriterqueue
    CPM_NAME readerwriterqueue
    CPM_VERSION 1.0.7
    CPM_GITHUB_REPOSITORY cameron314/readerwriterqueue
    CPM_GIT_TAG v1.0.7
)
argus_dep_end()

# Create argus::readerwriterqueue::readerwriterqueue alias
if(NOT TARGET argus::readerwriterqueue::readerwriterqueue)
  if(TARGET readerwriterqueue::readerwriterqueue)
    add_library(argus::readerwriterqueue::readerwriterqueue ALIAS readerwriterqueue::readerwriterqueue)
    argus_dep_log(SUCCESS "readerwriterqueue configured (readerwriterqueue::readerwriterqueue)")
  elseif(TARGET readerwriterqueue)
    add_library(argus::readerwriterqueue::readerwriterqueue ALIAS readerwriterqueue)
    argus_dep_log(SUCCESS "readerwriterqueue configured (readerwriterqueue)")
  elseif(DEFINED readerwriterqueue_SOURCE_DIR AND EXISTS "${readerwriterqueue_SOURCE_DIR}/readerwriterqueue.h")
    add_library(_argus_readerwriterqueue_header_only INTERFACE)
    target_include_directories(_argus_readerwriterqueue_header_only SYSTEM INTERFACE
            "${readerwriterqueue_SOURCE_DIR}"
        )
    add_library(argus::readerwriterqueue::readerwriterqueue ALIAS _argus_readerwriterqueue_header_only)
    argus_dep_log(SUCCESS "readerwriterqueue configured (header-only)")
  else()
    argus_dep_log(NOT_FOUND "readerwriterqueue")
  endif()
else()
  argus_dep_log(SUCCESS "readerwriterqueue configured (argus::readerwriterqueue::readerwriterqueue)")
endif()

# Create argus::readerwriterqueue convenience target
if(NOT TARGET _argus_readerwriterqueue_all)
  add_library(_argus_readerwriterqueue_all INTERFACE)
  if(TARGET argus::readerwriterqueue::readerwriterqueue)
    target_link_libraries(_argus_readerwriterqueue_all INTERFACE argus::readerwriterqueue::readerwriterqueue)
  endif()
endif()

if(NOT TARGET argus::readerwriterqueue)
  add_library(argus::readerwriterqueue ALIAS _argus_readerwriterqueue_all)
endif()
