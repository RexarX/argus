function(argus_add_example)
  cmake_parse_arguments(ARG "" "NAME" "SOURCES" ${ARGN})

  if(NOT ARG_NAME OR NOT ARG_SOURCES)
    message(FATAL_ERROR "argus_add_example: NAME and SOURCES are required")
  endif()

  set(_target "${ARG_NAME}_example")
  add_executable(${_target} ${ARG_SOURCES})

  argus_apply_conventions(${_target})
  argus_target_set_output_dirs(${_target} CUSTOM_FOLDER examples)
  argus_target_set_folder(${_target} "examples")

  target_link_libraries(${_target} PRIVATE argus::argus)
  if(ARGUS_ENABLE_EMBREE_BACKEND)
    argus_target_copy_embree_runtime(${_target})
  endif()
endfunction()

if(ARGUS_BUILD_SIM AND ARGUS_BUILD_ALGO)
  argus_add_example(
      NAME basic
      SOURCES basic.cpp
  )
endif()

if(ARGUS_BUILD_C_BINDINGS)
  add_subdirectory(bindings/c)
endif()

if(ARGUS_BUILD_PYTHON_BINDINGS)
  add_subdirectory(bindings/python)
endif()
