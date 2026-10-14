set(ARGUS_CORE_HEADERS
    include/argus/utils/format.hpp
    include/argus/utils/macro.hpp
    include/argus/assert.hpp
    include/argus/compiler.h
    include/argus/compiler.hpp
    include/argus/core.hpp
    include/argus/error.hpp
    include/argus/lidar.hpp
    include/argus/log.hpp
    include/argus/math.hpp
    include/argus/memory/aligned_alloc.hpp
    include/argus/memory/arena.hpp
    include/argus/memory/bump.hpp
    include/argus/memory/common.hpp
    include/argus/memory/offset.hpp
    include/argus/memory/temporary_storage.hpp
    include/argus/mirror.hpp
    include/argus/platform.h
    include/argus/platform.hpp
    include/argus/types.hpp
    include/argus/version.hpp
    include/argus/version_macros.h.in
)

set(ARGUS_CORE_SOURCES
    src/assert.cpp
    src/lidar.cpp
    src/log.cpp
    src/memory/aligned_alloc.cpp
    src/memory/arena.cpp
    src/memory/bump.cpp
    src/memory/offset.cpp
    src/memory/temporary_storage.cpp
    src/pch.hpp
    src/version.cpp
)

set(ARGUS_CORE_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/argus/version_macros.h.in"
    "${ARGUS_CORE_GENERATED_INCLUDE_DIR}/argus/version_macros.h"
    @ONLY
)

set(ARGUS_CORE_FILES
    ${ARGUS_CORE_HEADERS}
    ${ARGUS_CORE_SOURCES}
    "${ARGUS_CORE_GENERATED_INCLUDE_DIR}/argus/version_macros.h"
)

set(ARGUS_CORE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
set(ARGUS_CORE_GENERATED_INCLUDE_DIR "${ARGUS_CORE_GENERATED_INCLUDE_DIR}" PARENT_SCOPE)

add_library(argus_core STATIC ${ARGUS_CORE_FILES})
add_library(argus::core ALIAS argus_core)

set_target_properties(argus_core PROPERTIES
    OUTPUT_NAME argus_core
    EXPORT_NAME core
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

argus_apply_conventions(argus_core)
argus_target_set_output_dirs(argus_core)
argus_target_set_folder(argus_core "core")
argus_target_add_pch(argus_core "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp")

target_link_libraries(argus_core PUBLIC argus::lib::eigen)

if(ARGUS_USE_MIMALLOC)
  target_compile_definitions(argus_core PUBLIC ARGUS_USE_MIMALLOC)
  target_link_libraries(argus_core PRIVATE argus::lib::mimalloc)
endif()

if(ARGUS_OVERRIDE_GLOBAL_ALLOC)
  target_sources(argus_core PRIVATE src/memory/global_alloc.cpp)
  target_compile_definitions(argus_core PUBLIC ARGUS_OVERRIDE_GLOBAL_ALLOC)
endif()

target_compile_definitions(argus_core PUBLIC
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ARGUS_ENABLE_ASSERT>
)

target_include_directories(argus_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${ARGUS_CORE_GENERATED_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(ARGUS_ENABLE_INSTALL)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl"
  )
  install(FILES "${ARGUS_CORE_GENERATED_INCLUDE_DIR}/argus/version_macros.h"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/argus
  )
endif()
