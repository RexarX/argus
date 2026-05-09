if(ARGUS_BUILD_BINDINGS_CAPI)
  add_subdirectory(capi)
endif()

if(ARGUS_BUILD_BINDINGS_PYTHON)
  add_subdirectory(python)
endif()
