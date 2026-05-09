option(ARGUS_ALGO_BUILD_CLI "Build the argus algo CLI" ${PROJECT_IS_TOP_LEVEL})
set(MODULE_NAME "ALGO")

set(ARGUS_ALGO_HEADERS
)

set(ARGUS_ALGO_SOURCES
    src/library.cpp
)

set(ARGUS_ALGO_ALL_FILES
    ${ARGUS_ALGO_HEADERS}
    ${ARGUS_ALGO_SOURCES}
)

add_library(argus_algo STATIC ${ARGUS_ALGO_ALL_FILES})
add_library(argus::algo ALIAS argus_algo)

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_algo STANDARD 23)
argus_target_set_platform(argus_algo)
argus_target_set_output_dirs(argus_algo MODULE algo)
argus_target_set_folder(argus_algo "argus/algo")

include(${PROJECT_SOURCE_DIR}/cmake/Dependencies.cmake)
argus_require_dependency(AdaptiveCpp)

argus_target_add_pch(argus_algo ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
argus_target_set_warnings(argus_algo)
argus_target_set_optimization(argus_algo)
# LTO disabled: AdaptiveCpp compiles SYCL code to LLVM IR bitcode, which is
# incompatible with GNU archivers (gcc-ar). The syclcc-launcher handles
# link-time optimization for executables.
# argus_target_enable_lto(argus_algo)
set_target_properties(argus_algo PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF
    INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO OFF
)
argus_target_enable_sanitizers(argus_algo)

add_sycl_to_target(
    TARGET argus_algo
    SOURCES ${ARGUS_ALGO_ALL_FILES}
)

target_include_directories(argus_algo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

argus_require_dependency(eigen)

target_link_libraries(argus_algo PUBLIC
    argus::core
    argus::common
    argus::eigen::eigen
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(ARGUS_ALGO_BUILD_CLI)
  add_executable(argus_algo_executable
      ${ARGUS_ALGO_ALL_FILES}
      src/main.cpp
  )

  argus_target_set_cxx_standard(argus_algo_executable STANDARD 23)
  argus_target_set_platform(argus_algo_executable)
  argus_target_set_output_dirs(argus_algo_executable MODULE algo)
  argus_target_set_folder(argus_algo_executable "argus/algo_executable")

  argus_target_add_pch(argus_algo_executable ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
  argus_target_set_warnings(argus_algo_executable)
  argus_target_set_optimization(argus_algo_executable)
  argus_target_enable_lto(argus_algo_executable)
  argus_target_enable_sanitizers(argus_algo_executable)

  add_sycl_to_target(
      TARGET argus_algo_executable
      SOURCES ${ARGUS_ALGO_ALL_FILES} src/main.cpp
  )

  target_compile_options(argus_algo_executable PRIVATE -Wno-deprecated-declarations)
  target_include_directories(argus_algo_executable PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(argus_algo_executable PRIVATE argus::algo)
endif()
