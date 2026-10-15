cmake_minimum_required(VERSION 3.30)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version" FORCE)
endif()

project(argus
    VERSION 0.1.0
    DESCRIPTION "LiDAR and mirror parameter optimization"
    HOMEPAGE_URL "https://github.com/RexarX/argus"
    LANGUAGES C CXX
)

include(CMakeDependentOption)
include(GNUInstallDirs)

# Architecture
if(NOT (WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux" OR APPLE))
  message(FATAL_ERROR
      "Unsupported architecture or platform \"${CMAKE_SYSTEM_NAME}\", \"${CMAKE_SYSTEM_PROCESSOR}\""
  )
elseif((CMAKE_GENERATOR_PLATFORM MATCHES "ARM64") OR
       (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64"))
  set(BIN_ARCHITECTURE "arm64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(BIN_ARCHITECTURE "x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(BIN_ARCHITECTURE "x86")
else()
  message(FATAL_ERROR
      "Unsupported architecture or platform \"${CMAKE_SYSTEM_NAME}\", \"${CMAKE_SYSTEM_PROCESSOR}\""
  )
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(PROJECT_IS_TOP_LEVEL)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
    message(STATUS "Setting build type to 'Release' as none was specified.")
  endif()

  if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "" FORCE)
  endif()

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
  if(MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
      AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Ob2 /DNDEBUG /Zi"
        CACHE STRING "Flags used by the CXX compiler during RELWITHDEBINFO builds." FORCE)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /Ob2 /DNDEBUG /Zi"
        CACHE STRING "Flags used by the C compiler during RELWITHDEBINFO builds." FORCE)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG"
        CACHE STRING "Flags used by the CXX compiler during RELWITHDEBINFO builds." FORCE)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG"
        CACHE STRING "Flags used by the C compiler during RELWITHDEBINFO builds." FORCE)
  endif()
endif()

# Compatibility overrides for consumers that use the unprefixed names.
if(DEFINED DEVELOPER_MODE AND NOT DEFINED ARGUS_DEVELOPER_MODE)
  set(ARGUS_DEVELOPER_MODE "${DEVELOPER_MODE}" CACHE BOOL
      "Enable developer-focused build features (sanitizers)" FORCE)
endif()

option(ARGUS_BUILD_ALGO "Build the algorithm module" ON)
option(ARGUS_BUILD_SIM "Build the simulation module" ON)
cmake_dependent_option(ARGUS_BUILD_COMMON "Build the common module" ON
                       "ARGUS_BUILD_SIM" OFF)
cmake_dependent_option(ARGUS_BUILD_GUI "Build the GUI application"
                       ${PROJECT_IS_TOP_LEVEL}
                       "ARGUS_BUILD_ALGO;ARGUS_BUILD_SIM;ARGUS_BUILD_COMMON" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  cmake_dependent_option(ARGUS_GUI_NFD_WAYLAND
      "Linux GUI: nfd Wayland support (needs wayland-protocols XML at configure time)"
      ON
      "ARGUS_BUILD_GUI"
      OFF)
  cmake_dependent_option(ARGUS_GUI_NFD_X11
      "Linux GUI: nfd X11 support"
      ON
      "ARGUS_BUILD_GUI"
      OFF)
  cmake_dependent_option(ARGUS_GUI_NFD_USE_SYSTEM_WAYLAND_PROTOCOLS
      "Linux GUI: use installed wayland-protocols instead of downloading a copy"
      ON
      "ARGUS_BUILD_GUI;ARGUS_GUI_NFD_WAYLAND"
      OFF)
  set(ARGUS_WAYLAND_PROTOCOLS_DIR "" CACHE PATH
      "Directory containing wayland-protocols XML (e.g. /usr/share/wayland-protocols)"
  )
endif()
option(ARGUS_BUILD_C_BINDINGS "Build C bindings" OFF)
option(ARGUS_BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)
option(ARGUS_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(ARGUS_BUILD_SHARED "Build argus as a shared library" OFF)
option(ARGUS_DEVELOPER_MODE "Enable developer-focused build features (sanitizers)" OFF)
option(ARGUS_ENABLE_RELEASE_LTO "Enable Link Time Optimization for Release builds" ${PROJECT_IS_TOP_LEVEL})
option(ARGUS_ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(ARGUS_ENABLE_INSTALL "Enable installation rules" OFF)
option(ARGUS_ENABLE_EMBREE_BACKEND "Fetch and link the Embree CPU backend" ON)
option(ARGUS_USE_MIMALLOC "Allocate through mimalloc" ON)
option(ARGUS_OVERRIDE_GLOBAL_ALLOC "Replace global operator new and delete" ON)

cmake_dependent_option(
    ARGUS_ENABLE_LTO_RELWITHDEBINFO "Enable LTO for RelWithDebInfo" OFF
    "ARGUS_ENABLE_RELEASE_LTO" OFF
)
option(ARGUS_MANAGE_TOOLCHAIN
    "Allow argus to set CMAKE_LINKER / CMAKE_AR / CMAKE_RANLIB for the whole project"
    ${PROJECT_IS_TOP_LEVEL}
)

include(ClangArchiver)
argus_configure_clang_archiver()
include(Linker)
argus_configure_linker()

if(ARGUS_ENABLE_RELEASE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ARGUS_IPO_SUPPORTED OUTPUT error)
  if(ARGUS_IPO_SUPPORTED)
    set(ARGUS_IPO_SUPPORTED "${ARGUS_IPO_SUPPORTED}" CACHE INTERNAL "IPO/LTO support result")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    if(ARGUS_ENABLE_LTO_RELWITHDEBINFO)
      set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
      message(STATUS "Link-Time Optimization (LTO) enabled for Release and RelWithDebInfo builds")
    else()
      set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO OFF)
      message(STATUS "Link-Time Optimization (LTO) enabled for Release builds (RelWithDebInfo LTO off)")
    endif()
    argus_configure_lto_mode()
  else()
    message(STATUS "IPO/LTO not supported: ${error}")
    set(ARGUS_ENABLE_RELEASE_LTO OFF)
  endif()
  set(ARGUS_IPO_CACHED TRUE CACHE INTERNAL "IPO/LTO support has been checked globally")
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled for multi-config generators")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled on Linux")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled for ${CMAKE_BUILD_TYPE} builds")
endif()

include(Sanitizers)
include(TargetUtils)

argus_apply_linker()
argus_print_sanitizer_status()

set(ARGUS_ROOT_DIR ${PROJECT_SOURCE_DIR})

# Downloads live in <root>/.cpm_cache so every preset reuses the same sources.
# Per-preset FetchContent build trees stay under CMAKE_BINARY_DIR/_deps.
# CPM.cmake writes CPM_SOURCE_CACHE=OFF into the cache when unset; replace that
# default, but keep an explicit user or environment path.
if(DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_SOURCE_CACHE "$ENV{CPM_SOURCE_CACHE}" CACHE PATH
      "Directory to download CPM dependencies" FORCE
  )
elseif(NOT CPM_SOURCE_CACHE OR CPM_SOURCE_CACHE STREQUAL "OFF")
  set(CPM_SOURCE_CACHE "${ARGUS_ROOT_DIR}/.cpm_cache" CACHE PATH
      "Directory to download CPM dependencies" FORCE
  )
endif()
if(FETCHCONTENT_BASE_DIR STREQUAL "${ARGUS_ROOT_DIR}/build/_deps")
  unset(FETCHCONTENT_BASE_DIR CACHE)
endif()

include(CPM)
message(STATUS "CPM source cache: ${CPM_SOURCE_CACHE}")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Embree.cmake")
if(ARGUS_ENABLE_EMBREE_BACKEND)
  argus_setup_embree()
endif()

if(ARGUS_BUILD_PYTHON_BINDINGS)
  find_package(Python 3.9 COMPONENTS Interpreter Development.Module REQUIRED)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/pybind11.cmake")
  argus_setup_pybind11()
endif()

if(ARGUS_BUILD_COMMON)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Assimp.cmake")
  argus_setup_assimp()
endif()

if(ARGUS_BUILD_GUI)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT ARGUS_GUI_NFD_WAYLAND AND NOT ARGUS_GUI_NFD_X11)
      message(FATAL_ERROR
          "ARGUS_BUILD_GUI is ON: enable ARGUS_GUI_NFD_WAYLAND or ARGUS_GUI_NFD_X11"
      )
    endif()
  endif()
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Raylib.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ImGui.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RlImGui.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ImPlot.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ImGuizmo.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Nfd.cmake")
  argus_setup_raylib()
  argus_setup_imgui()
  argus_setup_rlimgui()
  argus_setup_implot()
  argus_setup_imguizmo()
  argus_setup_nfd()
endif()

add_subdirectory(third-party)
add_subdirectory(src)

if(ARGUS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(ARGUS_ENABLE_INSTALL)
  include(Install)
endif()
