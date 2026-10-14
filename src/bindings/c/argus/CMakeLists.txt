add_library(argus_c_argus INTERFACE)
add_library(argus::c::argus ALIAS argus_c_argus)

set_target_properties(argus_c_argus PROPERTIES EXPORT_NAME c::argus)
argus_target_set_folder(argus_c_argus "bindings/c")

target_include_directories(argus_c_argus INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Header-only. Link the modules argus.h includes so this target is enough
# to compile it. BUILD_INTERFACE keeps the installed umbrella from requiring
# the static archives when the shared library is what gets exported.
target_link_libraries(argus_c_argus INTERFACE $<BUILD_INTERFACE:argus::c::core>)
if(TARGET argus_c_algo)
  target_link_libraries(argus_c_argus INTERFACE $<BUILD_INTERFACE:argus::c::algo>)
endif()
if(TARGET argus_c_sim)
  target_link_libraries(argus_c_argus INTERFACE $<BUILD_INTERFACE:argus::c::sim>)
endif()

if(ARGUS_ENABLE_INSTALL)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      FILES_MATCHING PATTERN "*.h"
  )
endif()
