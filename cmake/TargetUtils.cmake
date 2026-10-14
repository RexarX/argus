# Per-target warning, optimization, LTO, platform, and output helpers.

include_guard(GLOBAL)

include(CheckIPOSupported)

# MSVC's default CMAKE_CXX_FLAGS include /EHsc /GR. Adding /EHs-c- /GR- on
# top of those produces D9025 override warnings on every translation unit.
if(MSVC)
  foreach(_argus_flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG
          CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO
          CMAKE_CXX_FLAGS_MINSIZEREL)
    if(DEFINED ${_argus_flag_var})
      string(REGEX REPLACE "(/|-)EH[a-zA-Z-]*" "" ${_argus_flag_var}
          "${${_argus_flag_var}}")
      string(REGEX REPLACE "(/|-)GR-?" "" ${_argus_flag_var}
          "${${_argus_flag_var}}")
    endif()
  endforeach()
  unset(_argus_flag_var)
endif()

function(argus_target_set_warnings TARGET)
  set(MSVC_WARNINGS
      /W4
      /w14242
      /w14254
      /w14263
      /w14265
      /w14287
      /we4289
      /w14296
      /w14311
      /w14545 /w14546 /w14547 /w14549 /w14555
      /w14619
      /w14640
      /w14826
      /w14905 /w14906
      /w14928
  )

  set(CLANG_WARNINGS
      -Wall
      -Wextra
      -Wpedantic
      -Wshadow
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Wcast-align
      -Wunused
      -Woverloaded-virtual
      -Wconversion
      -Wsign-conversion
      -Wnull-dereference
      -Wdouble-promotion
      -Wformat=2
      -Wimplicit-fallthrough
      -Wno-missing-field-initializers
  )

  set(GCC_WARNINGS
      ${CLANG_WARNINGS}
      -Wmisleading-indentation
      -Wduplicated-cond
      -Wduplicated-branches
      -Wlogical-op
      -Wuseless-cast
  )

  if(ARGUS_ENABLE_WARNINGS_AS_ERRORS)
    list(APPEND MSVC_WARNINGS /WX)
    list(APPEND CLANG_WARNINGS -Werror)
    list(APPEND GCC_WARNINGS -Werror)
  endif()

  target_compile_options(${TARGET} PRIVATE
      $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:${MSVC_WARNINGS}>
      $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<NOT:$<PLATFORM_ID:Windows>>>:${CLANG_WARNINGS}>
      $<$<CXX_COMPILER_ID:GNU>:${GCC_WARNINGS}>
  )
endfunction()

function(argus_target_set_optimization TARGET)
  set(_argus_msvc_debug_rtc "$<$<CONFIG:Debug>:/RTC1>")
  if(ARGUS_ENABLE_SANITIZERS AND ARGUS_SANITIZER_ADDRESS
      AND NOT ARGUS_COMPILER_IS_CLANG_CL)
    set(_argus_msvc_debug_rtc "")
  endif()

  target_compile_options(${TARGET} PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:
          /Zc:preprocessor
          /MP
      >
      $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:
          $<$<CONFIG:Debug>:/Od /Zi /MDd>
          ${_argus_msvc_debug_rtc}
          $<$<CONFIG:RelWithDebInfo>:/O2 /Ob2 /Zi /Zo /DNDEBUG>
          $<$<CONFIG:Release>:/O2 /Ob2 /DNDEBUG>
      >
      $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang,AppleClang>>,$<NOT:$<PLATFORM_ID:Windows>>>:
          $<$<CONFIG:Debug>:-Og -g3 -ggdb>
          $<$<CONFIG:RelWithDebInfo>:-O3 -g -fno-omit-frame-pointer -ffunction-sections -fdata-sections -fno-math-errno -DNDEBUG>
          $<$<CONFIG:Release>:-O3 -ffunction-sections -fdata-sections -fno-math-errno -DNDEBUG>
      >
      $<$<AND:$<PLATFORM_ID:Linux>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang,AppleClang>>>:
          $<$<CONFIG:Debug>:-gsplit-dwarf>
          $<$<CONFIG:RelWithDebInfo>:-gsplit-dwarf>
      >
      $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<NOT:$<PLATFORM_ID:Windows>>>:
          $<$<CONFIG:RelWithDebInfo>:-fdebug-info-for-profiling>
      >
  )

  target_link_options(${TARGET} PRIVATE
      $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang,AppleClang>>,$<NOT:$<PLATFORM_ID:Windows>>>:
          $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-rdynamic>
      >
      $<$<AND:$<PLATFORM_ID:Linux>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang,AppleClang>>>:
          $<$<CONFIG:RelWithDebInfo>:-Wl,--gc-sections>
      >
      $<$<AND:$<PLATFORM_ID:Darwin>,$<OR:$<CXX_COMPILER_ID:Clang,AppleClang>>>:
          $<$<CONFIG:RelWithDebInfo>:-Wl,-dead_strip>
      >
  )

  get_target_property(_target_type ${TARGET} TYPE)
  if(_target_type STREQUAL "EXECUTABLE" OR _target_type STREQUAL "SHARED_LIBRARY")
    target_link_options(${TARGET} PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:
            $<$<CONFIG:Debug>:/INCREMENTAL>
            $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO>
        >
    )
    if(NOT ARGUS_LINKER_RELWITHDEBINFO STREQUAL "rad")
      target_link_options(${TARGET} PRIVATE
          $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:
              $<$<CONFIG:RelWithDebInfo>:/OPT:REF /OPT:ICF>
          >
      )
    endif()
  endif()
endfunction()

function(argus_target_enable_lto TARGET)
  if(NOT ARGUS_ENABLE_RELEASE_LTO)
    return()
  endif()

  if(NOT DEFINED ARGUS_IPO_CACHED)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ARGUS_IPO_SUPPORTED OUTPUT _ipo_error LANGUAGES CXX)
    set(ARGUS_IPO_SUPPORTED "${ARGUS_IPO_SUPPORTED}" CACHE INTERNAL "IPO/LTO support result")
    set(ARGUS_IPO_CACHED TRUE CACHE INTERNAL "IPO/LTO support has been checked globally")
  endif()

  if(ARGUS_IPO_SUPPORTED)
    set_target_properties(${TARGET} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    )
    if(ARGUS_ENABLE_LTO_RELWITHDEBINFO)
      set_target_properties(${TARGET} PROPERTIES
          INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
      )
    else()
      set_target_properties(${TARGET} PROPERTIES
          INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO OFF
      )
    endif()
    if(COMMAND argus_target_apply_lto_mode)
      argus_target_apply_lto_mode(${TARGET})
    endif()
  endif()
endfunction()

function(argus_target_set_platform TARGET)
  target_compile_definitions(${TARGET} PRIVATE
      $<$<PLATFORM_ID:Windows>:ARGUS_PLATFORM_WINDOWS>
      $<$<PLATFORM_ID:Linux>:ARGUS_PLATFORM_LINUX>
      $<$<PLATFORM_ID:Darwin>:ARGUS_PLATFORM_MACOS>
      $<$<CONFIG:Debug>:ARGUS_DEBUG_MODE>
      $<$<CONFIG:RelWithDebInfo>:ARGUS_RELEASE_WITH_DEBUG_INFO_MODE>
      $<$<CONFIG:Release>:ARGUS_RELEASE_MODE NDEBUG>
  )

  if(WIN32)
    target_compile_definitions(${TARGET} PRIVATE
        UNICODE _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
  endif()
endfunction()

function(argus_target_set_shared_lib_platform TARGET)
  if(WIN32 AND MSVC)
    set_target_properties(${TARGET} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
  endif()
  set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

function(argus_target_set_output_dirs TARGET)
  cmake_parse_arguments(ARG "" "CUSTOM_FOLDER" "" ${ARGN})

  if(NOT PROJECT_IS_TOP_LEVEL)
    return()
  endif()

  if(NOT ARGUS_ROOT_DIR)
    set(ARGUS_ROOT_DIR "${PROJECT_SOURCE_DIR}")
  endif()

  if(NOT BIN_ARCHITECTURE)
    set(BIN_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
  endif()

  if(ARG_CUSTOM_FOLDER)
    set(_output_dir "${ARGUS_ROOT_DIR}/bin/${ARG_CUSTOM_FOLDER}/$<LOWER_CASE:$<CONFIG>>-$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>-$<LOWER_CASE:${BIN_ARCHITECTURE}>")
  else()
    set(_output_dir "${ARGUS_ROOT_DIR}/bin/$<LOWER_CASE:$<CONFIG>>-$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>-$<LOWER_CASE:${BIN_ARCHITECTURE}>")
  endif()

  set_target_properties(${TARGET} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${_output_dir}"
      LIBRARY_OUTPUT_DIRECTORY "${_output_dir}"
      ARCHIVE_OUTPUT_DIRECTORY "${_output_dir}"
  )
endfunction()

function(argus_target_disable_rtti_exceptions TARGET)
  target_compile_options(${TARGET} PRIVATE
      $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:
          /GR-
          /EHs-c-
      >
      $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang,AppleClang>>,$<NOT:$<PLATFORM_ID:Windows>>>:
          -fno-rtti
          -fno-exceptions
      >
  )
  target_compile_definitions(${TARGET} PRIVATE
      $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:
          _HAS_EXCEPTIONS=0
      >
  )
endfunction()

function(argus_target_set_cxx_standard TARGET)
  cmake_parse_arguments(ARG "" "STANDARD" "" ${ARGN})

  if(NOT ARG_STANDARD)
    set(ARG_STANDARD 23)
  endif()

  set_target_properties(${TARGET} PROPERTIES
      CXX_STANDARD ${ARG_STANDARD}
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
  )
endfunction()

function(argus_target_set_folder TARGET FOLDER_NAME)
  set_target_properties(${TARGET} PROPERTIES FOLDER ${FOLDER_NAME})
endfunction()

function(argus_target_add_pch TARGET PCH_FILE)
  cmake_parse_arguments(ARG "PUBLIC;INTERFACE" "" "" ${ARGN})

  set(VISIBILITY PRIVATE)
  if(ARG_PUBLIC)
    set(VISIBILITY PUBLIC)
  elseif(ARG_INTERFACE)
    set(VISIBILITY INTERFACE)
  endif()

  target_precompile_headers(${TARGET} ${VISIBILITY}
      $<$<COMPILE_LANGUAGE:CXX>:${PCH_FILE}>
  )
endfunction()

# Records enabled feature macros and defines them for targets created in the
# current directory and below. Umbrella targets still need these on their
# usage requirements so consumers can `#ifdef` them.
function(argus_add_enabled_feature_definitions)
  set(_defs "")
  if(ARGUS_ENABLE_EMBREE_BACKEND)
    list(APPEND _defs ARGUS_ENABLE_EMBREE_BACKEND)
  endif()
  if(ARGUS_BUILD_ALGO)
    list(APPEND _defs ARGUS_ENABLE_ALGO)
  endif()
  if(ARGUS_BUILD_SIM)
    list(APPEND _defs ARGUS_ENABLE_SIM)
  endif()

  if(_defs)
    add_compile_definitions(${_defs})
  endif()
  set(ARGUS_ENABLED_FEATURE_DEFINITIONS "${_defs}" PARENT_SCOPE)
endfunction()

#[[
    argus_apply_conventions(<target>
        [NO_WARNINGS] [NO_OPTIMIZATION] [NO_LTO] [NO_SANITIZERS]
        [NO_PLATFORM] [NO_LINKER] [ALLOW_EXCEPTIONS] [STANDARD <n>]
    )

    ALLOW_EXCEPTIONS keeps RTTI and exceptions (required by pybind11).
]]
function(argus_apply_conventions TARGET)
  cmake_parse_arguments(ARG
      "NO_WARNINGS;NO_OPTIMIZATION;NO_LTO;NO_SANITIZERS;NO_PLATFORM;NO_LINKER;ALLOW_EXCEPTIONS"
      "STANDARD"
      ""
      ${ARGN}
  )

  if(NOT TARGET ${TARGET})
    message(FATAL_ERROR "argus_apply_conventions: target \"${TARGET}\" does not exist")
  endif()

  if(NOT ARG_STANDARD)
    set(ARG_STANDARD 23)
  endif()

  argus_target_set_cxx_standard(${TARGET} STANDARD ${ARG_STANDARD})
  if(NOT ARG_ALLOW_EXCEPTIONS)
    argus_target_disable_rtti_exceptions(${TARGET})
  endif()

  if(NOT ARG_NO_PLATFORM)
    argus_target_set_platform(${TARGET})
  endif()
  if(NOT ARG_NO_OPTIMIZATION)
    argus_target_set_optimization(${TARGET})
  endif()
  if(NOT ARG_NO_WARNINGS)
    argus_target_set_warnings(${TARGET})
  endif()
  if(NOT ARG_NO_SANITIZERS)
    argus_target_enable_sanitizers(${TARGET})
  endif()
  if(NOT ARG_NO_LINKER AND COMMAND argus_target_apply_linker)
    argus_target_apply_linker(${TARGET})
  endif()
  if(NOT ARG_NO_LTO AND ARGUS_ENABLE_RELEASE_LTO)
    argus_target_enable_lto(${TARGET})
  endif()
endfunction()

function(_argus_glue_real_target OUT SRC)
  if(NOT TARGET ${SRC})
    set(${OUT} "" PARENT_SCOPE)
    return()
  endif()
  get_target_property(_aliased ${SRC} ALIASED_TARGET)
  if(_aliased)
    set(${OUT} ${_aliased} PARENT_SCOPE)
  else()
    set(${OUT} ${SRC} PARENT_SCOPE)
  endif()
endfunction()

# Expose a fetched or vendored dependency as argus::lib::<name> for in-tree
# links and argus package exports (EXPORT_NAME lib::<name>).
function(argus_register_lib_target NAME UPSTREAM)
  if(NOT TARGET ${UPSTREAM})
    message(FATAL_ERROR
        "argus_register_lib_target(${NAME}): '${UPSTREAM}' is not a target"
    )
  endif()
  if(TARGET argus::lib::${NAME})
    return()
  endif()
  _argus_glue_real_target(_real ${UPSTREAM})
  add_library(argus::lib::${NAME} ALIAS ${_real})
  get_target_property(_imported ${_real} IMPORTED)
  if(NOT _imported)
    set_target_properties(${_real} PROPERTIES EXPORT_NAME lib::${NAME})
  endif()
endfunction()

# Copy INTERFACE include dirs and compile defs from SRC onto DEST.
# LANG is C or CXX to wrap entries in $<COMPILE_LANGUAGE:...>; empty copies
# unfiltered (C API headers that both C and CXX consumers should see).
function(argus_glue_copy_usage DEST USAGE LANG SRC)
  if(NOT TARGET ${SRC})
    return()
  endif()

  get_target_property(_incs ${SRC} INTERFACE_INCLUDE_DIRECTORIES)
  if(_incs)
    foreach(_inc IN LISTS _incs)
      if(LANG)
        target_include_directories(${DEST} ${USAGE}
            "$<$<COMPILE_LANGUAGE:${LANG}>:${_inc}>"
        )
      else()
        target_include_directories(${DEST} ${USAGE} "${_inc}")
      endif()
    endforeach()
  endif()

  get_target_property(_defs ${SRC} INTERFACE_COMPILE_DEFINITIONS)
  if(_defs)
    foreach(_def IN LISTS _defs)
      if(LANG)
        target_compile_definitions(${DEST} ${USAGE}
            "$<$<COMPILE_LANGUAGE:${LANG}>:${_def}>"
        )
      else()
        target_compile_definitions(${DEST} ${USAGE} "${_def}")
      endif()
    endforeach()
  endif()
endfunction()

# Copy SRC's usage requirements and those of its PUBLIC link deps (skipping
# generator expressions so $<LINK_ONLY:...> / private BUILD_INTERFACE deps
# stay off the public include path). LANG is forwarded to
# argus_glue_copy_usage.
function(argus_glue_copy_usage_walk DEST USAGE LANG SRC)
  _argus_glue_real_target(_real ${SRC})
  if(NOT _real)
    return()
  endif()

  if(LANG)
    set(_prop "ARGUS_GLUE_USAGE_WALKED_${DEST}_${LANG}")
  else()
    set(_prop "ARGUS_GLUE_USAGE_WALKED_${DEST}_ANY")
  endif()
  get_property(_seen GLOBAL PROPERTY ${_prop})
  if(_real IN_LIST _seen)
    return()
  endif()
  set_property(GLOBAL APPEND PROPERTY ${_prop} ${_real})

  argus_glue_copy_usage(${DEST} ${USAGE} "${LANG}" ${_real})

  get_target_property(_libs ${_real} INTERFACE_LINK_LIBRARIES)
  if(NOT _libs)
    return()
  endif()
  foreach(_lib IN LISTS _libs)
    if(_lib MATCHES "^\\$<")
      continue()
    endif()
    argus_glue_copy_usage_walk(${DEST} ${USAGE} "${LANG}" ${_lib})
  endforeach()
endfunction()

# PRIVATE-link PUBLIC deps of MODULES that are not themselves modules
# (e.g. volk / VMA on a shared glue that WHOLEARCHIVE's backends).
function(argus_glue_link_nonmodule_deps DEST)
  set(_mods ${ARGN})
  set(_linked ${_mods})
  foreach(_mod IN LISTS _mods)
    if(NOT TARGET ${_mod})
      continue()
    endif()
    get_target_property(_libs ${_mod} INTERFACE_LINK_LIBRARIES)
    if(NOT _libs)
      continue()
    endif()
    foreach(_lib IN LISTS _libs)
      if(_lib MATCHES "^\\$<")
        continue()
      endif()
      if(NOT TARGET ${_lib})
        if(_lib AND NOT _lib IN_LIST _linked)
          list(APPEND _linked ${_lib})
          target_link_libraries(${DEST} PRIVATE ${_lib})
        endif()
        continue()
      endif()
      _argus_glue_real_target(_real ${_lib})
      if(NOT _real OR _real IN_LIST _linked)
        continue()
      endif()
      get_target_property(_type ${_real} TYPE)
      if(_type STREQUAL "INTERFACE_LIBRARY")
        list(APPEND _linked ${_real})
        continue()
      endif()
      list(APPEND _linked ${_real})
      target_link_libraries(${DEST} PRIVATE ${_real})
    endforeach()
  endforeach()
endfunction()
