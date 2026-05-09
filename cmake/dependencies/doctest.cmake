# doctest dependency configuration
#
# This module handles finding doctest from multiple sources:
# 1. System packages (pacman, apt, etc.)
# 2. CPM download (fallback)
#
# Usage:
#   argus_require_dependency(doctest)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "doctest" OUTPUT_VAR _doctest_processed)
if(_doctest_processed)
  return()
endif()

argus_dep_header(NAME "doctest")

# Use argus_module system for standard package finding
argus_dep_begin(
    NAME doctest
    VERSION ^2.0.0
    DEBIAN_NAMES doctest-dev
    RPM_NAMES doctest-devel
    PACMAN_NAMES doctest
    BREW_NAMES doctest
    PKG_CONFIG_NAMES doctest
    CPM_NAME doctest
    CPM_VERSION 2.5.2
    CPM_GITHUB_REPOSITORY doctest/doctest
    CPM_GIT_TAG v2.5.2
    CPM_OPTIONS
        "DOCTEST_WITH_TESTS OFF"
        "DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF"
)
argus_dep_end()

# Create argus::doctest::doctest alias if doctest was found
if(NOT TARGET argus::doctest::doctest)
  if(TARGET doctest::doctest)
    # Promote to GLOBAL before creating alias so it's visible from all scopes
    set_target_properties(doctest::doctest PROPERTIES IMPORTED_GLOBAL TRUE)
    # Check if it's an alias and get the real target
    get_target_property(_doctest_aliased doctest::doctest ALIASED_TARGET)
    if(_doctest_aliased)
      add_library(argus::doctest::doctest ALIAS ${_doctest_aliased})
    else()
      add_library(argus::doctest::doctest ALIAS doctest::doctest)
    endif()
    argus_dep_log(SUCCESS "doctest configured (doctest::doctest)")
  elseif(TARGET doctest)
    set_target_properties(doctest PROPERTIES IMPORTED_GLOBAL TRUE)
    # Check if it's an alias and get the real target
    get_target_property(_doctest_aliased doctest ALIASED_TARGET)
    if(_doctest_aliased)
      add_library(argus::doctest::doctest ALIAS ${_doctest_aliased})
    else()
      add_library(argus::doctest::doctest ALIAS doctest)
    endif()
    argus_dep_log(SUCCESS "doctest configured (doctest)")
  else()
    argus_dep_log(NOT_FOUND "doctest")
  endif()
else()
  argus_dep_log(SUCCESS "doctest configured (argus::doctest::doctest)")
endif()

# Create argus::doctest convenience target that brings in all doctest targets
if(NOT TARGET _argus_doctest_all)
  add_library(_argus_doctest_all INTERFACE)
  if(TARGET argus::doctest::doctest)
    target_link_libraries(_argus_doctest_all INTERFACE argus::doctest::doctest)
  endif()
endif()

if(NOT TARGET argus::doctest)
  add_library(argus::doctest ALIAS _argus_doctest_all)
endif()
