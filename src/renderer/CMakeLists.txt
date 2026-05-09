set(MODULE_NAME "RENDERER")
option(ARGUS_BUILD_RENDERER_WITH_ALGO "Link algo into renderer" ON)

set(ARGUS_RENDERER_HEADERS
)

set(ARGUS_RENDERER_SOURCES
    src/main.cpp
)

add_executable(argus_renderer
    ${ARGUS_RENDERER_HEADERS}
    ${ARGUS_RENDERER_SOURCES}
)

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_renderer STANDARD 23)
argus_target_set_platform(argus_renderer)
argus_target_set_output_dirs(argus_renderer MODULE renderer)
argus_target_set_folder(argus_renderer "argus/renderer")

argus_target_add_pch(argus_renderer ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
argus_target_set_warnings(argus_renderer)
argus_target_set_optimization(argus_renderer)
argus_target_enable_lto(argus_renderer)
argus_target_enable_sanitizers(argus_renderer)

target_include_directories(argus_renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include(${PROJECT_SOURCE_DIR}/cmake/Dependencies.cmake)
argus_require_dependency(eigen)
argus_require_dependency(argparse)
argus_require_dependency(glaze)

target_link_libraries(argus_renderer PRIVATE
    argus::core
    argus::common
    argus::eigen::eigen
    argus::argparse::argparse
    argus::glaze::glaze
)

if(ARGUS_BUILD_RENDERER_WITH_ALGO)
  target_compile_definitions(argus_renderer PRIVATE ARGUS_BUILD_RENDERER_WITH_ALGO)
  target_link_libraries(argus_renderer PRIVATE argus::algo)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
