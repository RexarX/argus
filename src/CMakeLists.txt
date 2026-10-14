argus_add_enabled_feature_definitions()

add_subdirectory(core)

if(ARGUS_BUILD_SIM)
  add_subdirectory(sim)
endif()

if(ARGUS_BUILD_ALGO)
  add_subdirectory(algo)
endif()

if(ARGUS_BUILD_COMMON)
  add_subdirectory(common)
endif()

if(ARGUS_BUILD_GUI)
  add_subdirectory(gui)
endif()

add_subdirectory(bindings)
add_subdirectory(argus)
