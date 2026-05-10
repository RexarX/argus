set(MODULE_NAME "CORE")

include(${PROJECT_SOURCE_DIR}/cmake/Dependencies.cmake)

set(ARGUS_CORE_HEADERS
    include/argus/assert.hpp
    include/argus/compiler.hpp
    include/argus/platform.hpp
    include/argus/stacktrace.hpp

    include/argus/container/static_string.hpp

    include/argus/log/config.hpp
    include/argus/log/logger.hpp

    include/argus/utils/filesystem.hpp
    include/argus/utils/hash.hpp
    include/argus/utils/macro.hpp
    include/argus/utils/type_info.hpp
)

set(ARGUS_CORE_SOURCES
    src/assert.cpp
    src/stacktrace.cpp

    src/log/logger.cpp
)

set(ARGUS_CORE_ALL_FILES
    ${ARGUS_CORE_HEADERS}
    ${ARGUS_CORE_SOURCES}
)

add_library(argus_core STATIC ${ARGUS_CORE_ALL_FILES})
add_library(argus::core ALIAS argus_core)

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_core STANDARD 23)
argus_target_set_platform(argus_core)
argus_target_set_output_dirs(argus_core MODULE core)
argus_target_set_folder(argus_core "argus/core")

argus_target_add_pch(argus_core ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
argus_target_set_warnings(argus_core)
argus_target_set_optimization(argus_core)
argus_target_enable_lto(argus_core)
argus_target_enable_sanitizers(argus_core)

target_compile_definitions(argus_core PUBLIC
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ARGUS_ENABLE_ASSERTS;ARGUS_ENABLE_STACKTRACE>
)

target_include_directories(argus_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

argus_require_dependency(Boost)

# Platform-specific stacktrace backend
if(ARGUS_USE_STL_STACKTRACE)
  message(STATUS "argus_core: Using C++23 STL stacktrace for Capture()")
  target_link_libraries(argus_core PUBLIC
      $<BUILD_INTERFACE:argus::stl_stacktrace>
  )
endif()

if(UNIX AND NOT APPLE)
  execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libbacktrace.a
      OUTPUT_VARIABLE BACKTRACE_LIBRARY_FROM_COMPILER
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
  )

  set(BACKTRACE_LIBRARY "")
  if(BACKTRACE_LIBRARY_FROM_COMPILER
        AND NOT BACKTRACE_LIBRARY_FROM_COMPILER STREQUAL "libbacktrace.a"
        AND EXISTS "${BACKTRACE_LIBRARY_FROM_COMPILER}")
    set(BACKTRACE_LIBRARY "${BACKTRACE_LIBRARY_FROM_COMPILER}")
  else()
    find_library(BACKTRACE_LIBRARY NAMES backtrace libbacktrace)
  endif()

  find_library(DL_LIBRARY NAMES dl)

  if(BACKTRACE_LIBRARY)
    target_link_libraries(argus_core PRIVATE ${BACKTRACE_LIBRARY})
    if(DL_LIBRARY)
      target_link_libraries(argus_core PRIVATE ${DL_LIBRARY})
    endif()
    get_filename_component(BACKTRACE_INCLUDE_DIR "${BACKTRACE_LIBRARY}" DIRECTORY)
    get_filename_component(BACKTRACE_INCLUDE_DIR "${BACKTRACE_INCLUDE_DIR}/include" ABSOLUTE)
    if(EXISTS "${BACKTRACE_INCLUDE_DIR}/backtrace.h")
      target_include_directories(argus_core PRIVATE ${BACKTRACE_INCLUDE_DIR})
    endif()
    target_compile_definitions(argus_core PRIVATE BOOST_STACKTRACE_USE_BACKTRACE)
  else()
    if(DL_LIBRARY)
      target_link_libraries(argus_core PRIVATE ${DL_LIBRARY})
    endif()
    target_compile_definitions(argus_core PRIVATE BOOST_STACKTRACE_USE_ADDR2LINE)
    message(STATUS "argus_core: libbacktrace not found, using Boost addr2line backend")
  endif()

  target_link_options(argus_core INTERFACE
      $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-rdynamic>
  )
elseif(APPLE)
  target_compile_definitions(argus_core PRIVATE BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
elseif(WIN32)
  target_compile_definitions(argus_core PRIVATE BOOST_STACKTRACE_USE_WINDBG)
endif()

target_compile_definitions(argus_core PRIVATE SPDLOG_HEADER_ONLY SPDLOG_USE_STD_FORMAT)

argus_require_dependency(spdlog)
target_link_libraries(argus_core PRIVATE argus::spdlog::spdlog_header_only)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
