cmake_minimum_required(VERSION 3.25)

project(argus_core
    VERSION 0.1.0
    DESCRIPTION "Argus core module"
    HOMEPAGE_URL "https://github.com/RexarX/argus"
    LANGUAGES C CXX
)

# Set archiver tools for Clang to support LTO
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    find_program(LLVM_LIB_EXECUTABLE
        NAMES llvm-lib llvm-lib.exe
        HINTS
            "$ENV{VCINSTALLDIR}/Tools/Llvm/x64/bin"
            "$ENV{VCINSTALLDIR}/Tools/Llvm/bin"
            "C:/Program Files/LLVM/bin"
            "C:/Program Files (x86)/LLVM/bin"
    )
    find_program(MSVC_LIB_EXECUTABLE NAMES lib lib.exe)

    if(LLVM_LIB_EXECUTABLE)
      set(CMAKE_AR "${LLVM_LIB_EXECUTABLE}" CACHE FILEPATH "Archiver" FORCE)
      message(STATUS "Using LLVM archiver for clang-cl: ${LLVM_LIB_EXECUTABLE}")
    elseif(MSVC_LIB_EXECUTABLE)
      set(CMAKE_AR "${MSVC_LIB_EXECUTABLE}" CACHE FILEPATH "Archiver" FORCE)
      message(STATUS "Using MSVC archiver for clang-cl: ${MSVC_LIB_EXECUTABLE}")
    else()
      message(WARNING "No MSVC-compatible archiver found for clang-cl. Build may fail.")
    endif()
    set(CMAKE_RANLIB "" CACHE FILEPATH "Ranlib" FORCE)
  else()
    find_program(LLVM_AR_EXECUTABLE llvm-ar)
    find_program(LLVM_RANLIB_EXECUTABLE llvm-ranlib)
    if(LLVM_AR_EXECUTABLE)
      set(CMAKE_AR "${LLVM_AR_EXECUTABLE}" CACHE FILEPATH "Archiver" FORCE)
    endif()
    if(LLVM_RANLIB_EXECUTABLE)
      set(CMAKE_RANLIB "${LLVM_RANLIB_EXECUTABLE}" CACHE FILEPATH "Ranlib" FORCE)
    endif()
  endif()
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "" FORCE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DEVELOPER_MODE "Enable developer-focused build features" OFF)
option(ARGUS_ENABLE_UNITY_BUILD "Enable Unity/Jumbo builds" OFF)
option(ARGUS_ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ${DEVELOPER_MODE})
option(ARGUS_BUILD_TESTS "Build argus core tests" ${PROJECT_IS_TOP_LEVEL})
option(ARGUS_BUILD_BENCHMARKS "Build argus benchmarks" ${PROJECT_IS_TOP_LEVEL})
option(ARGUS_BUILD_ALGO "Build argus algorithm" ON)
option(ARGUS_BUILD_BINDINGS "Build argus bindings" ${PROJECT_IS_TOP_LEVEL})
option(ARGUS_BUILD_RENDERER "Build argus renderer" ${PROJECT_IS_TOP_LEVEL})
if(NOT DEFINED BUILD_TESTING)
  set(BUILD_TESTING ${ARGUS_BUILD_TESTS} CACHE BOOL "Build tests")
endif()

if(ARGUS_ENABLE_UNITY_BUILD)
  set(CMAKE_UNITY_BUILD ON)
  set(CMAKE_UNITY_BUILD_BATCH_SIZE 16 CACHE STRING "Unity build batch size")
endif()

set(MODULE_NAME CORE)

set(ARGUS_${MODULE_NAME}_DIR "${CMAKE_CURRENT_BINARY_DIR}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers")
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(BuildDirHelper)

argus_print_build_dir_info()

include(CheckIPOSupported)
check_ipo_supported(RESULT ARGUS_IPO_SUPPORTED OUTPUT error)
if(ARGUS_IPO_SUPPORTED)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  message(STATUS "Link-Time Optimization (LTO) enabled for Release and RelWithDebInfo builds")

  # Clang uses ThinLTO which requires LLD instead of GNU ld
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT WIN32)
    set(LLD_NAMES)
    foreach(ver RANGE 99 10 -1)
      list(APPEND LLD_NAMES "lld-${ver}")
    endforeach()
    list(APPEND LLD_NAMES "lld")
    find_program(LLD NAMES ${LLD_NAMES})
    if(LLD)
      get_filename_component(LLD_NAME ${LLD} NAME)
      foreach(config RELEASE RELWITHDEBINFO)
        set(CMAKE_EXE_LINKER_FLAGS_${config}
            "${CMAKE_EXE_LINKER_FLAGS_${config}} -fuse-ld=${LLD_NAME}")
        set(CMAKE_SHARED_LINKER_FLAGS_${config}
            "${CMAKE_SHARED_LINKER_FLAGS_${config}} -fuse-ld=${LLD_NAME}")
        set(CMAKE_MODULE_LINKER_FLAGS_${config}
            "${CMAKE_MODULE_LINKER_FLAGS_${config}} -fuse-ld=${LLD_NAME}")
      endforeach()
      message(STATUS "LTO: Using LLD linker (${LLD_NAME}) for ThinLTO compatibility")
    else()
      message(WARNING "LTO enabled with Clang but LLD not found. ThinLTO may fail with GNU ld.")
    endif()
  endif()
else()
  message(STATUS "IPO/LTO not supported: ${error}")
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled for multi-config generators")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled for ${CMAKE_BUILD_TYPE} builds")
endif()

include(cmake/Sanitizers.cmake)
argus_print_sanitizer_status()

if(BUILD_TESTING)
  include(CTest)
  enable_testing()
endif()

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(ARGUS_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

argus_print_dependency_summary()
