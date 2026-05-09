add_subdirectory(core)
add_subdirectory(common)

if(ARGUS_BUILD_ALGO)
  add_subdirectory(algo)
endif()

if(ARGUS_BUILD_BINDINGS)
  add_subdirectory(bindings)
endif()

if(ARGUS_BUILD_RENDERER)
  add_subdirectory(renderer)
endif()
