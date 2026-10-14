function(argus_add_c_module)
  cmake_parse_arguments(ARG "" "NAME;CPP_TARGET" "SOURCES;HEADERS" ${ARGN})

  if(NOT ARG_NAME)
    message(FATAL_ERROR "argus_add_c_module: NAME is required")
  endif()

  set(_target "argus_c_${ARG_NAME}")
  set(_files ${ARG_HEADERS} ${ARG_SOURCES})
  if(NOT _files)
    message(FATAL_ERROR "argus_add_c_module(${ARG_NAME}): SOURCES/HEADERS are required")
  endif()

  add_library(${_target} STATIC ${_files})
  add_library(argus::c::${ARG_NAME} ALIAS ${_target})

  set_target_properties(${_target} PROPERTIES
      OUTPUT_NAME ${_target}
      EXPORT_NAME c::${ARG_NAME}
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
      C_EXTENSIONS OFF
      CXX_VISIBILITY_PRESET hidden
      C_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN ON
      POSITION_INDEPENDENT_CODE ON
  )

  argus_apply_conventions(${_target})
  argus_target_set_output_dirs(${_target})
  argus_target_set_folder(${_target} "bindings/c")

  argus_target_add_pch(${_target} "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp")

  target_include_directories(${_target}
      PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
      PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  if(NOT ARG_NAME STREQUAL "core")
    target_link_libraries(${_target} PUBLIC argus::c::core)
  endif()

  if(ARG_CPP_TARGET)
    target_link_libraries(${_target} PRIVATE ${ARG_CPP_TARGET})
    set_property(GLOBAL APPEND PROPERTY ARGUS_C_CPP_DEPS ${ARG_CPP_TARGET})
  endif()

  if(ARGUS_ENABLE_INSTALL)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )
  endif()

  set_property(GLOBAL APPEND PROPERTY ARGUS_C_TARGETS ${_target})
endfunction()

add_subdirectory(core)

if(ARGUS_BUILD_ALGO)
  add_subdirectory(algo)
endif()

if(ARGUS_BUILD_SIM)
  add_subdirectory(sim)
endif()

add_subdirectory(argus)

get_property(_argus_c_targets GLOBAL PROPERTY ARGUS_C_TARGETS)
get_property(_argus_c_cpp_deps GLOBAL PROPERTY ARGUS_C_CPP_DEPS)

add_library(argus_c INTERFACE)
add_library(argus::c ALIAS argus_c)
set_target_properties(argus_c PROPERTIES EXPORT_NAME c)
argus_target_set_folder(argus_c "bindings/c")

foreach(_mod IN LISTS _argus_c_targets)
  target_link_libraries(argus_c INTERFACE "$<LINK_ONLY:${_mod}>")
  argus_glue_copy_usage(argus_c INTERFACE "" ${_mod})
endforeach()

foreach(_cpp IN LISTS _argus_c_cpp_deps)
  target_link_libraries(argus_c INTERFACE "$<LINK_ONLY:${_cpp}>")
endforeach()

target_link_libraries(argus_c INTERFACE argus::c::argus)
argus_glue_copy_usage(argus_c INTERFACE "" argus_c_argus)

if(ARGUS_ENABLED_FEATURE_DEFINITIONS)
  target_compile_definitions(argus_c INTERFACE ${ARGUS_ENABLED_FEATURE_DEFINITIONS})
endif()
