set(MODULE_NAME "COMMON")

set(ARGUS_COMMON_HEADERS
    include/argus/common/scene.hpp

    include/argus/common/config/argparse_policy.hpp
    include/argus/common/config/argus.hpp
    include/argus/common/config/config.hpp
    include/argus/common/config/lidar.hpp
    include/argus/common/config/mirror.hpp
    include/argus/common/config/optimizer.hpp
    include/argus/common/config/paths.hpp
    include/argus/common/config/policy.hpp
    include/argus/common/config/toml_policy.hpp
)

set(ARGUS_COMMON_SOURCES
    src/config/argparse_policy.cpp
    src/config/argus.cpp
    src/config/config.cpp
    src/config/lidar.cpp
    src/config/mirror.cpp
    src/config/optimizer.cpp
    src/config/paths.cpp
    src/config/toml_policy.cpp
)

set(ARGUS_COMMON_ALL_FILES
    ${ARGUS_COMMON_HEADERS}
    ${ARGUS_COMMON_SOURCES}
)

add_library(argus_common STATIC ${ARGUS_COMMON_ALL_FILES})
add_library(argus::common ALIAS argus_common)

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_common STANDARD 23)
argus_target_set_platform(argus_common)
argus_target_set_output_dirs(argus_common MODULE common)
argus_target_set_folder(argus_common "argus/common")

argus_target_add_pch(argus_common ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
argus_target_set_warnings(argus_common)
argus_target_set_optimization(argus_common)
argus_target_enable_lto(argus_common)
argus_target_enable_sanitizers(argus_common)

target_include_directories(argus_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include(${PROJECT_SOURCE_DIR}/cmake/Dependencies.cmake)
argus_require_dependency(eigen)
argus_require_dependency(glaze)
argus_require_dependency(argparse)
argus_require_dependency(assimp)

target_link_libraries(argus_common
    PUBLIC
        argus::core
        argus::eigen::eigen
        argus::glaze::glaze
    PRIVATE
        argus::argparse::argparse
        argus::assimp::assimp
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
