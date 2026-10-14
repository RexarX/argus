set(ARGUS_ALGO_HEADERS
    include/argus/algo.hpp
    include/argus/bound.hpp
    include/argus/context.hpp
    include/argus/coverage.hpp
    include/argus/optimize.hpp
    include/argus/target.hpp
    include/argus/workspace.hpp
)

set(ARGUS_ALGO_SOURCES
    src/coverage.cpp
    src/optimize.cpp
    src/pch.hpp
    src/target.cpp
)

add_library(argus_algo STATIC ${ARGUS_ALGO_HEADERS} ${ARGUS_ALGO_SOURCES})
add_library(argus::algo ALIAS argus_algo)

set_target_properties(argus_algo PROPERTIES
    OUTPUT_NAME argus_algo
    EXPORT_NAME algo
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

argus_apply_conventions(argus_algo)
argus_target_set_output_dirs(argus_algo)
argus_target_set_folder(argus_algo "algo")
argus_target_add_pch(argus_algo "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp")

target_include_directories(argus_algo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(argus_algo PUBLIC argus::core)
if(TARGET argus_sim)
  target_link_libraries(argus_algo PUBLIC argus::sim)
endif()

if(ARGUS_ENABLE_INSTALL)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl"
  )
endif()
