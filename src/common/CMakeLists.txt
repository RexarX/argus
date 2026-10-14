set(ARGUS_COMMON_HEADERS
    include/argus/common.hpp
    include/argus/component_pool.hpp
    include/argus/entity.hpp
    include/argus/entity_manager.hpp
    include/argus/mesh_io.hpp
)

set(ARGUS_COMMON_SOURCES
    src/entity_manager.cpp
    src/mesh_io.cpp
)

add_library(argus_common STATIC ${ARGUS_COMMON_HEADERS} ${ARGUS_COMMON_SOURCES})
add_library(argus::common ALIAS argus_common)

set_target_properties(argus_common PROPERTIES
    OUTPUT_NAME argus_common
    EXPORT_NAME common
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

argus_apply_conventions(argus_common)
argus_target_set_output_dirs(argus_common)
argus_target_set_folder(argus_common "common")
argus_target_add_pch(argus_common "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp")

target_include_directories(argus_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(argus_common
    PUBLIC argus::core
    PRIVATE argus::lib::assimp
)

if(ARGUS_ENABLE_INSTALL)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl"
  )
endif()
