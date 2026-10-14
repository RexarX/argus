# Umbrella over core, algo, sim, and common when those modules are enabled.
# Static builds keep one archive per module; this target carries usage
# requirements and argus.hpp. Shared builds produce one library.
# C bindings use argus::c; Python uses separate extension modules.

set(ARGUS_UMBRELLA_HEADERS
    include/argus/argus.hpp
)

set(_argus_cxx_modules argus_core)
if(TARGET argus_algo)
  list(APPEND _argus_cxx_modules argus_algo)
endif()
if(TARGET argus_sim)
  list(APPEND _argus_cxx_modules argus_sim)
endif()
if(TARGET argus_common)
  list(APPEND _argus_cxx_modules argus_common)
endif()

set(_argus_modules ${_argus_cxx_modules})

if(ARGUS_BUILD_SHARED)
  set(_argus_glue_src "${CMAKE_CURRENT_BINARY_DIR}/argus_glue.cpp")
  if(NOT EXISTS "${_argus_glue_src}")
    file(WRITE "${_argus_glue_src}" "/* argus::argus shared glue */\n")
  endif()

  add_library(argus SHARED "${_argus_glue_src}" ${ARGUS_UMBRELLA_HEADERS})
  argus_target_set_shared_lib_platform(argus)
  target_compile_definitions(argus
      PUBLIC ARGUS_BUILD_SHARED
      PRIVATE ARGUS_EXPORTS
  )
  set_target_properties(argus PROPERTIES
      OUTPUT_NAME argus
      EXPORT_NAME argus
      VERSION ${PROJECT_VERSION}
      SOVERSION ${PROJECT_VERSION_MAJOR}
      CXX_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN ON
  )
  argus_apply_conventions(argus)
  argus_target_set_output_dirs(argus)
  argus_target_set_folder(argus "argus")

  foreach(_mod IN LISTS _argus_modules)
    target_compile_definitions(${_mod} PRIVATE
        ARGUS_BUILD_SHARED
        ARGUS_EXPORTS
    )
    set_target_properties(${_mod} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        C_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
    add_dependencies(argus ${_mod})
    if(MSVC)
      target_link_options(argus PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${_mod}>")
    else()
      target_link_libraries(argus PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,${_mod}>")
    endif()
  endforeach()

  argus_glue_link_nonmodule_deps(argus ${_argus_modules})
  if(ARGUS_ENABLE_EMBREE_BACKEND)
    argus_target_link_embree(argus)
    argus_target_copy_embree_runtime(argus)
  endif()
else()
  add_library(argus INTERFACE)
  set_target_properties(argus PROPERTIES EXPORT_NAME argus)
  argus_target_set_folder(argus "argus")

  foreach(_mod IN LISTS _argus_modules)
    target_link_libraries(argus INTERFACE "$<LINK_ONLY:${_mod}>")
  endforeach()
  if(ARGUS_ENABLE_EMBREE_BACKEND)
    argus_target_link_embree(argus)
  endif()
endif()

add_library(argus::argus ALIAS argus)

if(ARGUS_BUILD_SHARED)
  set(_argus_usage PUBLIC)
else()
  set(_argus_usage INTERFACE)
endif()

# C++ usage requirements come from enabled C++ modules. LINK_ONLY would
# otherwise hide them. C consumers must not receive those paths.
target_include_directories(argus ${_argus_usage}
    "$<$<COMPILE_LANGUAGE:CXX>:$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>>"
    "$<$<COMPILE_LANGUAGE:CXX>:$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>>"
)

set_property(GLOBAL PROPERTY ARGUS_GLUE_USAGE_WALKED_argus_CXX "")
foreach(_mod IN LISTS _argus_cxx_modules)
  argus_glue_copy_usage_walk(argus ${_argus_usage} CXX ${_mod})
endforeach()

if(ARGUS_ENABLED_FEATURE_DEFINITIONS)
  target_compile_definitions(argus ${_argus_usage} ${ARGUS_ENABLED_FEATURE_DEFINITIONS})
endif()

if(ARGUS_ENABLE_INSTALL)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      FILES_MATCHING PATTERN "*.hpp"
  )
endif()
