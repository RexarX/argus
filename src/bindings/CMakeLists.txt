option(ARGUS_BUILD_BINDINGS_CAPI "Build CAPI bindings" ON)
option(ARGUS_BUILD_BINDINGS_PYTHON "Build Python bindings" ON)

if(ARGUS_BUILD_BINDINGS_CAPI)
  add_subdirectory(capi)
endif()

if(ARGUS_BUILD_BINDINGS_PYTHON)
  add_subdirectory(python)
endif()
