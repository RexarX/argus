if(ARGUS_BUILD_C_BINDINGS)
  add_subdirectory(c)
endif()

if(ARGUS_BUILD_PYTHON_BINDINGS)
  add_subdirectory(python)
endif()
