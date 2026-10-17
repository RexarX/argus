set(ARGUS_SIM_HEADERS
    include/argus/sim/backend.hpp
    include/argus/sim/instance.hpp
    include/argus/sim/scan.hpp
    include/argus/sim/scene.hpp
    include/argus/sim/sim.hpp
)

set(ARGUS_SIM_SOURCES
    src/backend.cpp
    src/callback.cpp
    src/dispatch.hpp
    src/instance.cpp
    src/pch.hpp
    src/scene.cpp
    src/scan.cpp
)

if(ARGUS_ENABLE_EMBREE_BACKEND)
  list(APPEND ARGUS_SIM_HEADERS
      include/argus/sim/embree/instance.hpp
      include/argus/sim/embree/scene.hpp
  )
  list(APPEND ARGUS_SIM_SOURCES
      src/embree/instance.cpp
  )
endif()

add_library(argus_sim STATIC ${ARGUS_SIM_HEADERS} ${ARGUS_SIM_SOURCES})
add_library(argus::sim ALIAS argus_sim)

set_target_properties(argus_sim PROPERTIES
    OUTPUT_NAME argus_sim
    EXPORT_NAME sim
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

argus_apply_conventions(argus_sim)
argus_target_set_output_dirs(argus_sim)
argus_target_set_folder(argus_sim "sim")
argus_target_add_pch(argus_sim "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp")

target_include_directories(argus_sim
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(argus_sim PUBLIC argus::core)
if(ARGUS_ENABLE_EMBREE_BACKEND)
  argus_target_link_embree(argus_sim)
endif()

if(ARGUS_ENABLE_INSTALL)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl"
  )
endif()
