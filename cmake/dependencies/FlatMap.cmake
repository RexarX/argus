# FlatMap dependency configuration
#
# This module handles finding flat_map from multiple sources:
# 1. C++23 std::flat_map (if available)
# 2. Boost.Container flat_map (fallback)
#
# Usage:
#   argus_require_dependency(FlatMap)

include_guard(GLOBAL)

# Check if already processed
argus_dep_is_processed(NAME "FlatMap" OUTPUT_VAR _flatmap_processed)
if(_flatmap_processed)
  return()
endif()

argus_dep_header(NAME "FlatMap")

# Check if C++23 std::flat_map is available
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")

check_cxx_source_compiles("
#include <flat_map>
#include <string>

int main() {
    std::flat_map<int, std::string> map;
    map.insert({1, \"test\"});
    auto it = map.find(1);
    return it != map.end() ? 0 : 1;
}
" argus_HAS_STL_FLAT_MAP)

cmake_pop_check_state()

if(argus_HAS_STL_FLAT_MAP)
  argus_dep_log(SUCCESS "C++23 std::flat_map available, using STL flat_map")
  set(argus_USE_STL_FLAT_MAP ON CACHE INTERNAL "Use C++23 STL flat_map instead of Boost")

  # Create a target for STL flat_map
  if(NOT TARGET argus::stl_flat_map)
    add_library(argus::stl_flat_map INTERFACE IMPORTED GLOBAL)
    target_compile_definitions(argus::stl_flat_map INTERFACE argus_USE_STL_FLAT_MAP)
  endif()

  # Create argus::flat_map as alias to STL flat_map
  if(NOT TARGET argus::flat_map)
    add_library(argus::flat_map INTERFACE IMPORTED GLOBAL)
    target_link_libraries(argus::flat_map INTERFACE argus::stl_flat_map)
  endif()

  argus_dep_mark_found(NAME "FlatMap" VIA "STL (C++23)")
  argus_dep_mark_processed(NAME "FlatMap")
else()
  argus_dep_log(STATUS "C++23 std::flat_map not available, using Boost.Container flat_map")
  set(argus_USE_STL_FLAT_MAP OFF CACHE INTERNAL "Use C++23 STL flat_map instead of Boost")

  # Require Boost dependency (this will handle finding/downloading Boost)
  argus_require_dependency(Boost)

  # Check if Boost was found
  if(TARGET Boost::boost OR TARGET argus::boost::boost)
    # Create argus::boost::container target for flat_map
    if(NOT TARGET argus::boost::container)
      add_library(argus::boost::container INTERFACE IMPORTED GLOBAL)

      if(TARGET Boost::container)
        target_link_libraries(argus::boost::container INTERFACE Boost::container)
      elseif(TARGET argus::boost::boost)
        # Boost.Container is header-only for flat_map
        target_link_libraries(argus::boost::container INTERFACE argus::boost::boost)

        # Add container include path if using CPM-downloaded Boost
        if(Boost_SOURCE_DIR AND EXISTS "${Boost_SOURCE_DIR}/libs/container/include")
          target_include_directories(argus::boost::container SYSTEM INTERFACE
                        "${Boost_SOURCE_DIR}/libs/container/include"
                    )
        endif()
      endif()
    endif()

    # Create argus::flat_map as alias to Boost container
    if(NOT TARGET argus::flat_map)
      add_library(argus::flat_map INTERFACE IMPORTED GLOBAL)
      target_link_libraries(argus::flat_map INTERFACE argus::boost::container)
      if(TARGET argus::boost::boost)
        target_link_libraries(argus::flat_map INTERFACE argus::boost::boost)
      endif()
    endif()

    argus_dep_mark_found(NAME "FlatMap" VIA "Boost.Container")
    argus_dep_mark_processed(NAME "FlatMap")
  else()
    argus_dep_log(WARNING "FlatMap: Neither std::flat_map nor Boost.Container available")
    argus_dep_mark_processed(NAME "FlatMap")
  endif()
endif()
