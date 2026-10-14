# Official Embree 4.4.1 CPU binaries.
#
# https://github.com/RenderKit/embree/releases/tag/v4.4.1
# The .sycl packages from that release are not used.

include_guard(GLOBAL)

set(ARGUS_EMBREE_VERSION "4.4.1")

function(_argus_embree_asset OUT_NAME OUT_HASH)
  if(WIN32)
    if(NOT BIN_ARCHITECTURE STREQUAL "x64")
      message(FATAL_ERROR
          "Embree ${ARGUS_EMBREE_VERSION} publishes Windows x64 binaries only "
          "(this build is ${BIN_ARCHITECTURE})."
      )
    endif()
    if(NOT MSVC AND NOT (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
      message(FATAL_ERROR
          "Embree's Windows binaries are MSVC import libraries. "
          "Configure with the MSVC or clang-cl preset."
      )
    endif()
    set(${OUT_NAME} "embree-${ARGUS_EMBREE_VERSION}.x64.windows.zip" PARENT_SCOPE)
    set(${OUT_HASH}
        "5aa1fa3161a720f9c610b06c652b80f421cb2b1fe8221fdf92b74157336fdf8f"
        PARENT_SCOPE)
    return()
  endif()

  if(APPLE)
    if(BIN_ARCHITECTURE STREQUAL "arm64")
      set(${OUT_NAME} "embree-${ARGUS_EMBREE_VERSION}.arm64.macosx.zip" PARENT_SCOPE)
      set(${OUT_HASH}
          "12236c79f37e98224582f7fa9e113160b3d4eb5eb5f24846a96faf05e32a5562"
          PARENT_SCOPE)
      return()
    endif()
    if(BIN_ARCHITECTURE STREQUAL "x64")
      set(${OUT_NAME} "embree-${ARGUS_EMBREE_VERSION}.x86_64.macosx.zip" PARENT_SCOPE)
      set(${OUT_HASH}
          "bbbfc0a1beea14c157d17349b71151b846baad884d81fb5da6d74fb8b0bacaec"
          PARENT_SCOPE)
      return()
    endif()
    message(FATAL_ERROR
        "Embree ${ARGUS_EMBREE_VERSION} has no macOS binary for "
        "${BIN_ARCHITECTURE}."
    )
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT BIN_ARCHITECTURE STREQUAL "x64")
      message(FATAL_ERROR
          "Embree ${ARGUS_EMBREE_VERSION} publishes Linux x86_64 binaries only "
          "(this build is ${BIN_ARCHITECTURE})."
      )
    endif()
    set(${OUT_NAME} "embree-${ARGUS_EMBREE_VERSION}.x86_64.linux.tar.gz" PARENT_SCOPE)
    set(${OUT_HASH}
        "aa506a55a581055e30cdc8819557dd4163ed514fc63ce4c1d66a6df82631be3f"
        PARENT_SCOPE)
    return()
  endif()

  message(FATAL_ERROR
      "Embree ${ARGUS_EMBREE_VERSION} has no binary for ${CMAKE_SYSTEM_NAME}."
  )
endfunction()

function(_argus_embree_root_name ASSET OUT_VAR)
  if(ASSET MATCHES "\\.tar\\.gz$")
    string(LENGTH "${ASSET}" _length)
    math(EXPR _keep "${_length} - 7")
    string(SUBSTRING "${ASSET}" 0 ${_keep} _name)
  else()
    string(REGEX REPLACE "\\.zip$" "" _name "${ASSET}")
  endif()
  set(${OUT_VAR} "${_name}" PARENT_SCOPE)
endfunction()

function(_argus_embree_collect_runtime PREFIX OUT_VAR)
  set(_files "")
  if(WIN32)
    foreach(_name IN ITEMS embree4.dll tbb12.dll tbbmalloc.dll)
      set(_path "${PREFIX}/bin/${_name}")
      if(NOT EXISTS "${_path}")
        message(FATAL_ERROR "Embree package is missing ${_path}")
      endif()
      list(APPEND _files "${_path}")
    endforeach()
  elseif(APPLE)
    file(GLOB _files
        "${PREFIX}/lib/libembree4*.dylib"
        "${PREFIX}/lib/libtbb*.dylib"
        "${PREFIX}/lib/libtbbmalloc*.dylib"
    )
    list(REMOVE_DUPLICATES _files)
  else()
    file(GLOB _files
        "${PREFIX}/lib/libembree4.so*"
        "${PREFIX}/lib/libtbb.so*"
        "${PREFIX}/lib/libtbbmalloc.so*"
    )
    list(REMOVE_DUPLICATES _files)
  endif()

  if(NOT _files)
    message(FATAL_ERROR
        "Embree package at \"${PREFIX}\" has no runtime libraries"
    )
  endif()
  set(${OUT_VAR} "${_files}" PARENT_SCOPE)
endfunction()

#[[
    argus_setup_embree()

    Downloads the v4.4.1 CPU archive into CPM_SOURCE_CACHE and imports the
    embree target. Release binaries are mapped onto Debug and RelWithDebInfo.
]]
function(argus_setup_embree)
  get_property(_ready GLOBAL PROPERTY ARGUS_EMBREE_PREFIX)
  if(_ready AND TARGET embree)
    return()
  endif()

  if(NOT CPM_SOURCE_CACHE)
    message(FATAL_ERROR "argus_setup_embree: CPM_SOURCE_CACHE is not set")
  endif()

  _argus_embree_asset(_asset _hash)
  _argus_embree_root_name("${_asset}" _root_name)

  set(_cache "${CPM_SOURCE_CACHE}/embree")
  set(_archive "${_cache}/${_asset}")
  set(_root "${_cache}/${_root_name}")
  set(_config "${_root}/lib/cmake/embree-${ARGUS_EMBREE_VERSION}/embree-config.cmake")
  set(_url
      "https://github.com/RenderKit/embree/releases/download/v${ARGUS_EMBREE_VERSION}/${_asset}"
  )

  if(EXISTS "${_archive}")
    file(SHA256 "${_archive}" _actual)
    if(NOT _actual STREQUAL "${_hash}")
      message(STATUS "Embree: checksum mismatch, downloading ${_asset} again")
      file(REMOVE "${_archive}")
    endif()
  endif()

  if(NOT EXISTS "${_archive}")
    message(STATUS "Embree: downloading ${_asset}")
    file(DOWNLOAD "${_url}" "${_archive}"
        EXPECTED_HASH "SHA256=${_hash}"
        SHOW_PROGRESS
        TLS_VERIFY ON
        TIMEOUT 600
        INACTIVITY_TIMEOUT 60
        STATUS _status
    )
    list(GET _status 0 _code)
    if(NOT _code EQUAL 0)
      list(GET _status 1 _message)
      message(FATAL_ERROR "Embree download failed (${_code}): ${_message}")
    endif()
  endif()

  if(NOT EXISTS "${_config}")
    message(STATUS "Embree: extracting ${_asset}")
    file(MAKE_DIRECTORY "${_cache}")
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_cache}")
  endif()

  if(NOT EXISTS "${_config}")
    message(FATAL_ERROR "Embree package is missing ${_config}")
  endif()

  set(embree_DIR "${_root}/lib/cmake/embree-${ARGUS_EMBREE_VERSION}"
      CACHE PATH "Directory containing embree-config.cmake" FORCE)
  find_package(embree ${ARGUS_EMBREE_VERSION} CONFIG REQUIRED)
  if(NOT TARGET embree)
    message(FATAL_ERROR "Embree was found but did not define the embree target")
  endif()

  set_target_properties(embree PROPERTIES
      MAP_IMPORTED_CONFIG_DEBUG Release
      MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
      MAP_IMPORTED_CONFIG_MINSIZEREL Release
  )

  _argus_embree_collect_runtime("${_root}" _runtime)
  set(_names "")
  foreach(_file IN LISTS _runtime)
    cmake_path(GET _file FILENAME _filename)
    list(APPEND _names "${_filename}")
  endforeach()

  set_property(GLOBAL PROPERTY ARGUS_EMBREE_PREFIX "${_root}")
  set_property(GLOBAL PROPERTY ARGUS_EMBREE_RUNTIME_FILES "${_runtime}")
  set_property(GLOBAL PROPERTY ARGUS_EMBREE_RUNTIME_NAMES "${_names}")
  set_property(GLOBAL PROPERTY ARGUS_EMBREE_PACKAGE_DIR
      "${CMAKE_INSTALL_LIBDIR}/embree/lib/cmake/embree-${ARGUS_EMBREE_VERSION}")

  argus_register_lib_target(embree embree)
  message(STATUS "Embree: ${embree_VERSION} (${_root})")
endfunction()

#[[
    argus_target_link_embree(<target>)

    Links the fetched Embree import library. Static libraries keep the
    dependency private to the build tree so install(EXPORT) does not require
    the imported target.
]]
function(argus_target_link_embree TARGET)
  if(NOT TARGET argus::lib::embree)
    message(FATAL_ERROR "argus_target_link_embree: call argus_setup_embree() first")
  endif()
  if(NOT TARGET ${TARGET})
    message(FATAL_ERROR "argus_target_link_embree: target \"${TARGET}\" does not exist")
  endif()

  get_target_property(_type ${TARGET} TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY")
    target_link_libraries(${TARGET} INTERFACE
        "$<BUILD_INTERFACE:$<LINK_ONLY:argus::lib::embree>>"
    )
  else()
    target_link_libraries(${TARGET} PRIVATE
        "$<BUILD_INTERFACE:argus::lib::embree>"
    )
  endif()
endfunction()

#[[
    argus_target_copy_embree_runtime(<target>)

    Copies Embree and TBB shared libraries next to an executable or module.
]]
function(argus_target_copy_embree_runtime TARGET)
  if(NOT ARGUS_ENABLE_EMBREE_BACKEND)
    return()
  endif()
  if(NOT TARGET ${TARGET})
    message(FATAL_ERROR
        "argus_target_copy_embree_runtime: target \"${TARGET}\" does not exist"
    )
  endif()

  get_target_property(_type ${TARGET} TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY" OR _type STREQUAL "STATIC_LIBRARY")
    return()
  endif()

  get_property(_files GLOBAL PROPERTY ARGUS_EMBREE_RUNTIME_FILES)
  if(NOT _files)
    return()
  endif()

  foreach(_file IN LISTS _files)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_file}"
            "$<TARGET_FILE_DIR:${TARGET}>"
        VERBATIM
    )
  endforeach()

  if(UNIX)
    if(APPLE)
      set(_origin "@loader_path")
    else()
      set(_origin "\$ORIGIN")
    endif()
    set_property(TARGET ${TARGET} APPEND PROPERTY BUILD_RPATH "${_origin}")
    set_property(TARGET ${TARGET} APPEND PROPERTY INSTALL_RPATH "${_origin}")
  endif()
endfunction()

#[[
    argus_install_embree()

    Installs the fetched prefix under <libdir>/embree so its cmake package
    keeps the relative paths baked into embree-config.cmake.
]]
function(argus_install_embree)
  if(NOT ARGUS_ENABLE_INSTALL)
    return()
  endif()

  get_property(_prefix GLOBAL PROPERTY ARGUS_EMBREE_PREFIX)
  if(NOT _prefix)
    return()
  endif()

  set(_dest "${CMAKE_INSTALL_LIBDIR}/embree")
  install(DIRECTORY "${_prefix}/include/" DESTINATION "${_dest}/include")
  install(DIRECTORY "${_prefix}/lib/" DESTINATION "${_dest}/lib")

  if(WIN32)
    get_property(_runtime GLOBAL PROPERTY ARGUS_EMBREE_RUNTIME_FILES)
    install(FILES ${_runtime} DESTINATION "${_dest}/bin")
    install(FILES ${_runtime} DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endif()
endfunction()
