# Boost dependency configuration
#
# This module handles finding Boost from multiple sources:
# 1. System packages (pacman, apt, etc.)
# 2. CPM download (fallback)
#
# Additionally, it checks for C++23 <stacktrace> header availability
# and uses STL stacktrace instead of Boost if available.
#
# Usage:
#   argus_require_dependency(Boost)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "Boost" OUTPUT_VAR _boost_processed)
if(_boost_processed)
  return()
endif()

argus_dep_header(NAME "Boost")

# Check if C++23 <stacktrace> header is available
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_REQUIRED_LINK_OPTIONS "")

# Try to detect the compiler and set appropriate flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # GCC requires libstdc++_libbacktrace for std::stacktrace
  list(APPEND CMAKE_REQUIRED_LINK_OPTIONS "-lstdc++_libbacktrace")
  set(_stl_stacktrace_link_libs "stdc++_libbacktrace")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # Clang with libc++ may need different handling
  if(CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
    # libc++ doesn't have full stacktrace support yet in most versions
    set(_stl_stacktrace_link_libs "")
  else()
    # Clang with libstdc++
    list(APPEND CMAKE_REQUIRED_LINK_OPTIONS "-lstdc++_libbacktrace")
    set(_stl_stacktrace_link_libs "stdc++_libbacktrace")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # MSVC has built-in support, no extra libs needed
  set(_stl_stacktrace_link_libs "")
endif()

check_cxx_source_compiles("
#include <stacktrace>
#include <string>

int main() {
    auto st = std::stacktrace::current();
    if (st.size() > 0) {
        std::string desc = std::to_string(st[0]);
        (void)desc;
    }
    return 0;
}
" ARGUS_HAS_STL_STACKTRACE)

cmake_pop_check_state()

if(ARGUS_HAS_STL_STACKTRACE)
  argus_dep_log(SUCCESS "C++23 <stacktrace> header available, using STL stacktrace for Capture()")
  set(ARGUS_USE_STL_STACKTRACE ON CACHE INTERNAL "Use C++23 STL stacktrace instead of Boost")

  # Create a target for STL stacktrace
  if(NOT TARGET argus::stl_stacktrace)
    add_library(argus::stl_stacktrace INTERFACE IMPORTED GLOBAL)
    target_compile_definitions(argus::stl_stacktrace INTERFACE ARGUS_USE_STL_STACKTRACE)

    # Link required libraries for STL stacktrace
    if(_stl_stacktrace_link_libs)
      target_link_libraries(argus::stl_stacktrace INTERFACE ${_stl_stacktrace_link_libs})
    endif()
  endif()
else()
  argus_dep_log(STATUS "C++23 <stacktrace> not available, using Boost stacktrace")
  set(ARGUS_USE_STL_STACKTRACE OFF CACHE INTERNAL "Use C++23 STL stacktrace instead of Boost")
endif()

# Boost components required by Argus
# Note: unordered is header-only and doesn't need to be in this list
# stacktrace is always required (for boost::stacktrace::from_current_exception
# even when STL <stacktrace> is available)
set(ARGUS_BOOST_REQUIRED_COMPONENTS
      stacktrace
  )

# Minimum Boost version for proper boost::stacktrace::from_current_exception support
if(WIN32)
  set(ARGUS_BOOST_MIN_VERSION 1.86)
else()
  set(ARGUS_BOOST_MIN_VERSION 1.85)
endif()

# Try to find Boost in order of preference
set(_boost_found_via "")
set(_boost_prefer_cpm FALSE)

# If Boost was resolved via CPM in this build tree before, skip repeated
# expensive system probing on subsequent configure runs.
if(DEFINED ARGUS_DEP_BOOST_FOUND_VIA AND ARGUS_DEP_BOOST_FOUND_VIA STREQUAL "CPM")
  set(_boost_prefer_cpm TRUE)
endif()

# If Boost targets already exist (e.g., from a parent project or prior
# find_package), skip finding/downloading Boost entirely. This prevents
# ALIAS target conflicts in colcon workspaces where multiple packages
# independently try to add Boost via CPM.
if(TARGET Boost::boost OR TARGET Boost::headers)
  set(_boost_already_exists TRUE)
else()
  set(_boost_already_exists FALSE)
endif()

if(NOT _boost_already_exists)
  # 1. Try system package manager first
  # Try CONFIG mode first (modern CMake packages like Arch Linux, Ubuntu 22.04+)
  if(NOT _boost_prefer_cpm)
    find_package(Boost ${ARGUS_BOOST_MIN_VERSION} QUIET CONFIG)
    if(Boost_FOUND)
      set(_boost_found_via "system (CONFIG)")
    else()
      # Fall back to MODULE mode with specific components
      if(ARGUS_BOOST_REQUIRED_COMPONENTS)
        find_package(Boost ${ARGUS_BOOST_MIN_VERSION} QUIET COMPONENTS ${ARGUS_BOOST_REQUIRED_COMPONENTS})
      else()
        find_package(Boost ${ARGUS_BOOST_MIN_VERSION} QUIET)
      endif()
      if(Boost_FOUND)
        set(_boost_found_via "system (MODULE)")
      endif()
    endif()
  endif()
endif()

if(_boost_already_exists)
  # Boost targets already exist (from CPM in a prior colcon package or parent project).
  # Create our argus::boost:: aliases pointing to the existing Boost targets.
  # The CPM add_subdirectory has already been done by another package in this workspace.
  #
  # If Boost was discovered earlier without COMPONENTS (e.g. plain
  # find_package(Boost)), stacktrace component targets may be missing even
  # though Boost itself is available. Request stacktrace components explicitly
  # so argus::boost::stacktrace resolves to real link libraries.
  if(NOT TARGET Boost::stacktrace
       AND NOT TARGET Boost::stacktrace_backtrace
       AND NOT TARGET Boost::stacktrace_addr2line
       AND NOT TARGET Boost::stacktrace_basic
       AND NOT TARGET Boost::stacktrace_from_exception
       AND NOT TARGET Boost::stacktrace_noop)
    find_package(Boost QUIET COMPONENTS
        stacktrace
        stacktrace_backtrace
        stacktrace_addr2line
        stacktrace_basic
        stacktrace_from_exception
        stacktrace_noop
    )
  endif()

  argus_dep_log(SUCCESS "Boost targets already exist, creating aliases")
  argus_dep_mark_found(NAME "Boost" VIA "existing")

  if(NOT TARGET argus::boost::boost)
    add_library(argus::boost::boost INTERFACE IMPORTED GLOBAL)
    if(TARGET Boost::boost)
      target_link_libraries(argus::boost::boost INTERFACE Boost::boost)
    endif()
  endif()

  if(NOT TARGET argus::boost::stacktrace)
    add_library(argus::boost::stacktrace INTERFACE IMPORTED GLOBAL)
    if(TARGET Boost::stacktrace)
      target_link_libraries(argus::boost::stacktrace INTERFACE Boost::stacktrace)
    elseif(TARGET Boost::stacktrace_backtrace)
      target_link_libraries(argus::boost::stacktrace INTERFACE Boost::stacktrace_backtrace)
    elseif(TARGET argus::boost::boost)
      target_link_libraries(argus::boost::stacktrace INTERFACE argus::boost::boost)
    endif()
  endif()

  if(TARGET Boost::stacktrace_basic AND NOT TARGET argus::boost::stacktrace_basic)
    add_library(argus::boost::stacktrace_basic INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_basic INTERFACE Boost::stacktrace_basic)
  endif()

  if(TARGET Boost::stacktrace_backtrace AND NOT TARGET argus::boost::stacktrace_backtrace)
    add_library(argus::boost::stacktrace_backtrace INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_backtrace INTERFACE Boost::stacktrace_backtrace)
  endif()

  if(TARGET Boost::stacktrace_addr2line AND NOT TARGET argus::boost::stacktrace_addr2line)
    add_library(argus::boost::stacktrace_addr2line INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_addr2line INTERFACE Boost::stacktrace_addr2line)
  endif()

  if(TARGET Boost::stacktrace_from_exception AND NOT TARGET argus::boost::stacktrace_from_exception)
    add_library(argus::boost::stacktrace_from_exception INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_from_exception INTERFACE Boost::stacktrace_from_exception)
  endif()

  if(NOT TARGET argus::boost::stacktrace_from_exception AND TARGET Boost::stacktrace)
    add_library(argus::boost::stacktrace_from_exception INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_from_exception INTERFACE Boost::stacktrace)
  endif()

  if(TARGET Boost::stacktrace_noop AND NOT TARGET argus::boost::stacktrace_noop)
    add_library(argus::boost::stacktrace_noop INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_noop INTERFACE Boost::stacktrace_noop)
  endif()

  if(NOT TARGET argus::boost::unordered)
    add_library(argus::boost::unordered INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::unordered INTERFACE Boost::boost)
  endif()

  if(TARGET Boost::container AND NOT TARGET argus::boost::container)
    add_library(argus::boost::container INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::container INTERFACE Boost::container)
  endif()

  argus_dep_mark_processed(NAME "Boost")

elseif(Boost_FOUND)
  # 3. Create Boost targets if found via system package
  if(_boost_found_via)
    argus_dep_log(SUCCESS "Boost found via ${_boost_found_via}")
    argus_dep_mark_found(NAME "Boost" VIA "${_boost_found_via}")
  else()
    argus_dep_log(SUCCESS "Boost found at ${Boost_DIR}")
    argus_dep_mark_found(NAME "Boost" VIA "system")
  endif()

  # Create convenience target: argus::boost::boost (main headers)
  if(NOT TARGET argus::boost::boost)
    add_library(argus::boost::boost INTERFACE IMPORTED)
    target_link_libraries(argus::boost::boost INTERFACE Boost::boost)
  endif()

  # Create stacktrace target: argus::boost::stacktrace always links to Boost::stacktrace
  if(NOT TARGET argus::boost::stacktrace)
    add_library(argus::boost::stacktrace INTERFACE IMPORTED GLOBAL)
    if(TARGET Boost::stacktrace)
      target_link_libraries(argus::boost::stacktrace INTERFACE Boost::stacktrace)
    elseif(TARGET argus::boost::boost)
      target_link_libraries(argus::boost::stacktrace INTERFACE argus::boost::boost)
    endif()
  endif()

  if(TARGET Boost::stacktrace_basic AND NOT TARGET argus::boost::stacktrace_basic)
    add_library(argus::boost::stacktrace_basic INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_basic INTERFACE Boost::stacktrace_basic)
  endif()

  if(TARGET Boost::stacktrace_backtrace AND NOT TARGET argus::boost::stacktrace_backtrace)
    add_library(argus::boost::stacktrace_backtrace INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_backtrace INTERFACE Boost::stacktrace_backtrace)
  endif()

  if(TARGET Boost::stacktrace_addr2line AND NOT TARGET argus::boost::stacktrace_addr2line)
    add_library(argus::boost::stacktrace_addr2line INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_addr2line INTERFACE Boost::stacktrace_addr2line)
  endif()

  if(TARGET Boost::stacktrace_from_exception AND NOT TARGET argus::boost::stacktrace_from_exception)
    add_library(argus::boost::stacktrace_from_exception INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_from_exception INTERFACE Boost::stacktrace_from_exception)
  endif()

  if(NOT TARGET argus::boost::stacktrace_from_exception AND TARGET Boost::stacktrace)
    add_library(argus::boost::stacktrace_from_exception INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_from_exception INTERFACE Boost::stacktrace)
  endif()

  if(TARGET Boost::stacktrace_noop AND NOT TARGET argus::boost::stacktrace_noop)
    add_library(argus::boost::stacktrace_noop INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_noop INTERFACE Boost::stacktrace_noop)
  endif()

  if(TARGET Boost::stacktrace_windbg AND NOT TARGET argus::boost::stacktrace_windbg)
    add_library(argus::boost::stacktrace_windbg INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::stacktrace_windbg INTERFACE Boost::stacktrace_windbg)
  endif()

  # Create alias for header-only unordered (not a separate component)
  if(NOT TARGET argus::boost::unordered)
    add_library(argus::boost::unordered INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::boost::unordered INTERFACE Boost::boost)
  endif()

  # Mark as processed
  argus_dep_mark_processed(NAME "Boost")
else()
  # 4. Try CPM fallback if system packages not found
  if(ARGUS_DOWNLOAD_PACKAGES)
    argus_dep_log(DOWNLOAD "Boost not found in system, downloading via CPM...")

    # Always include stacktrace
    # (needed for boost::stacktrace::stacktrace::from_current_exception)
    # Always include container for flat_map support
    set(BOOST_INCLUDE_LIBRARIES "container;stacktrace;unordered" CACHE STRING "" FORCE)

    include(DownloadUsingCPM)
    argus_cpm_add_package(
        NAME Boost
        VERSION 1.91.0
        URL https://github.com/boostorg/boost/releases/download/boost-1.91.0-1/boost-1.91.0-1-cmake.tar.xz
        OPTIONS
            "BOOST_ENABLE_CMAKE ON"
            "BUILD_SHARED_LIBS OFF"
            "CMAKE_POSITION_INDEPENDENT_CODE ON"
        SYSTEM
    )

    # Create aliases if Boost was just added or already cached
    if(Boost_ADDED OR TARGET Boost::boost)
      if(NOT TARGET argus::boost::boost)
        add_library(argus::boost::boost INTERFACE IMPORTED GLOBAL)
        if(TARGET Boost::boost)
          target_link_libraries(argus::boost::boost INTERFACE Boost::boost)
        endif()
      endif()

      # Always create Boost stacktrace target (needed for from_current_exception even with STL stacktrace)
      if(NOT TARGET argus::boost::stacktrace)
        add_library(argus::boost::stacktrace INTERFACE IMPORTED GLOBAL)
      endif()

      if(TARGET Boost::stacktrace)
        target_link_libraries(argus::boost::stacktrace INTERFACE Boost::stacktrace)
      else()
        target_link_libraries(argus::boost::stacktrace INTERFACE argus::boost::boost)
      endif()

      if(TARGET Boost::stacktrace_backtrace AND NOT TARGET argus::boost::stacktrace_backtrace)
        add_library(argus::boost::stacktrace_backtrace INTERFACE IMPORTED GLOBAL)
        target_link_libraries(argus::boost::stacktrace_backtrace INTERFACE Boost::stacktrace_backtrace)
      endif()

      if(TARGET Boost::stacktrace_addr2line AND NOT TARGET argus::boost::stacktrace_addr2line)
        add_library(argus::boost::stacktrace_addr2line INTERFACE IMPORTED GLOBAL)
        target_link_libraries(argus::boost::stacktrace_addr2line INTERFACE Boost::stacktrace_addr2line)
      endif()

      if(TARGET Boost::stacktrace_from_exception AND NOT TARGET argus::boost::stacktrace_from_exception)
        add_library(argus::boost::stacktrace_from_exception INTERFACE IMPORTED GLOBAL)
        target_link_libraries(argus::boost::stacktrace_from_exception INTERFACE Boost::stacktrace_from_exception)
      endif()

      if(NOT TARGET argus::boost::stacktrace_from_exception AND TARGET Boost::stacktrace)
        add_library(argus::boost::stacktrace_from_exception INTERFACE IMPORTED GLOBAL)
        target_link_libraries(argus::boost::stacktrace_from_exception INTERFACE Boost::stacktrace)
      endif()

      if(TARGET Boost::stacktrace_noop AND NOT TARGET argus::boost::stacktrace_noop)
        add_library(argus::boost::stacktrace_noop INTERFACE IMPORTED GLOBAL)
        target_link_libraries(argus::boost::stacktrace_noop INTERFACE Boost::stacktrace_noop)
      endif()

      # Create alias for header-only unordered (not a separate component)
      if(NOT TARGET argus::boost::unordered)
        add_library(argus::boost::unordered INTERFACE IMPORTED GLOBAL)
        target_link_libraries(argus::boost::unordered INTERFACE argus::boost::boost)

        # Manually add unordered include path since BOOST_INCLUDE_LIBRARIES doesn't always set it up
        if(Boost_SOURCE_DIR AND EXISTS "${Boost_SOURCE_DIR}/libs/unordered/include")
          target_include_directories(argus::boost::unordered SYSTEM INTERFACE
                        "${Boost_SOURCE_DIR}/libs/unordered/include"
                    )
        endif()
      endif()

      # Create alias for container (needed for flat_map)
      if(NOT TARGET argus::boost::container)
        add_library(argus::boost::container INTERFACE IMPORTED GLOBAL)
        if(TARGET Boost::container)
          target_link_libraries(argus::boost::container INTERFACE Boost::container)
        else()
          target_link_libraries(argus::boost::container INTERFACE argus::boost::boost)
        endif()

        # Manually add container include path since BOOST_INCLUDE_LIBRARIES doesn't always set it up
        if(Boost_SOURCE_DIR AND EXISTS "${Boost_SOURCE_DIR}/libs/container/include")
          target_include_directories(argus::boost::container SYSTEM INTERFACE
                        "${Boost_SOURCE_DIR}/libs/container/include"
                    )
        endif()
      endif()

      # Mark Boost targets as SYSTEM to suppress warnings
      if(TARGET Boost::boost)
        argus_cpm_mark_as_system(Boost::boost)
      endif()
      if(TARGET Boost::stacktrace)
        argus_cpm_mark_as_system(Boost::stacktrace)
      endif()
      if(TARGET Boost::stacktrace_backtrace)
        argus_cpm_mark_as_system(Boost::stacktrace_backtrace)
      endif()
      if(TARGET Boost::stacktrace_addr2line)
        argus_cpm_mark_as_system(Boost::stacktrace_addr2line)
      endif()
      if(TARGET Boost::stacktrace_basic)
        argus_cpm_mark_as_system(Boost::stacktrace_basic)
      endif()
      if(TARGET Boost::container)
        argus_cpm_mark_as_system(Boost::container)
      endif()
    endif()

    # Mark as found and processed
    argus_dep_mark_processed(NAME "Boost")
    argus_dep_mark_found(NAME "Boost" VIA "CPM")
    list(APPEND CPM_PACKAGES "Boost")
  else()
    argus_dep_log(WARNING "Boost not found and ARGUS_DOWNLOAD_PACKAGES is OFF")
    argus_dep_mark_processed(NAME "Boost")
  endif()
endif()

# Dynamically create aliases for each Boost component (only if not using STL stacktrace for stacktrace component)
foreach(component IN LISTS ARGUS_BOOST_REQUIRED_COMPONENTS)
  set(target_name "argus::boost::${component}")

  if(TARGET "Boost::${component}")
    if(NOT TARGET ${target_name})
      add_library(${target_name} INTERFACE IMPORTED GLOBAL)
      target_link_libraries(${target_name} INTERFACE "Boost::${component}")
      argus_dep_log(STATUS "Created alias: ${target_name}")
    endif()
    # Link header-only style components to argus::boost::boost.
    # Avoid linking stacktrace components here to prevent leaking
    # BOOST_STACKTRACE_* link macros into unrelated targets.
    if(NOT component STREQUAL "pool"
         AND NOT component STREQUAL "container"
         AND NOT component STREQUAL "stacktrace"
         AND NOT component STREQUAL "stacktrace_backtrace"
         AND NOT component STREQUAL "stacktrace_addr2line"
         AND NOT component STREQUAL "stacktrace_basic"
         AND NOT component STREQUAL "stacktrace_from_exception"
         AND NOT component STREQUAL "stacktrace_noop")
      target_link_libraries(argus::boost::boost INTERFACE "Boost::${component}")
    endif()
  elseif(NOT TARGET ${target_name})
    # The argus::boost::stacktrace target may already be created from
    # individual backend targets (backtrace, addr2line, etc.)
    argus_dep_log(WARNING "Boost component '${component}' not found")
  endif()
endforeach()

# Create argus::boost convenience target that brings in all Boost targets
if(NOT TARGET _ARGUS_boost_all)
  add_library(_ARGUS_boost_all INTERFACE)
  if(TARGET argus::boost::boost)
    target_link_libraries(_ARGUS_boost_all INTERFACE argus::boost::boost)
  endif()
  if(TARGET argus::boost::unordered)
    target_link_libraries(_ARGUS_boost_all INTERFACE argus::boost::unordered)
  endif()
  if(TARGET argus::boost::stacktrace)
    target_link_libraries(_ARGUS_boost_all INTERFACE argus::boost::stacktrace)
  endif()
endif()

if(NOT TARGET argus::boost)
  add_library(argus::boost ALIAS _ARGUS_boost_all)
endif()
