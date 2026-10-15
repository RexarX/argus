# nativefiledialog-extended 1.4.1, static.
#
# https://github.com/btzy/nativefiledialog-extended/releases/tag/v1.4.1

include_guard(GLOBAL)
include(CPM)
include("${CMAKE_CURRENT_LIST_DIR}/WaylandProtocols.cmake")

set(ARGUS_NFD_VERSION "1.4.1")
set(ARGUS_NFD_URL
    "https://github.com/btzy/nativefiledialog-extended/archive/refs/tags/v${ARGUS_NFD_VERSION}.zip"
)
set(ARGUS_NFD_URL_HASH
    "SHA256=ffd177323a36c313de0ef0187f956aa65f32f12639f86fb204b5c8558e0a22f6"
)

function(_argus_nfd_cache_root OUT_VAR)
  if(NOT CPM_SOURCE_CACHE)
    message(FATAL_ERROR "argus_setup_nfd: CPM_SOURCE_CACHE is not set")
  endif()
  set(${OUT_VAR} "${CPM_SOURCE_CACHE}/nfd/v${ARGUS_NFD_VERSION}" PARENT_SCOPE)
endfunction()

function(_argus_nfd_migrate_legacy_cache NEW_ROOT)
  if(EXISTS "${NEW_ROOT}/src/CMakeLists.txt")
    return()
  endif()
  if(NOT CPM_SOURCE_CACHE)
    return()
  endif()

  file(GLOB _legacy LIST_DIRECTORIES true "${CPM_SOURCE_CACHE}/nfd/*")
  foreach(_dir IN LISTS _legacy)
    if(_dir STREQUAL "${NEW_ROOT}")
      continue()
    endif()
    if(EXISTS "${_dir}/src/CMakeLists.txt")
      message(STATUS "nfd: reusing sources from ${_dir}")
      file(COPY "${_dir}/" DESTINATION "${NEW_ROOT}")
      return()
    endif()
  endforeach()
endfunction()

function(_argus_nfd_download_archive ROOT)
  file(MAKE_DIRECTORY "${ROOT}")
  if(EXISTS "${ROOT}/src/CMakeLists.txt")
    return()
  endif()

  message(STATUS "nfd: downloading v${ARGUS_NFD_VERSION} into ${ROOT}")
  set(_zip "${ROOT}/.nfd-src.zip")
  file(DOWNLOAD "${ARGUS_NFD_URL}" "${_zip}" EXPECTED_HASH "${ARGUS_NFD_URL_HASH}"
      SHOW_PROGRESS TLS_VERIFY ON
  )
  file(ARCHIVE_EXTRACT INPUT "${_zip}" DESTINATION "${ROOT}/.nfd-extract")
  file(REMOVE "${_zip}")

  file(GLOB _tops LIST_DIRECTORIES true "${ROOT}/.nfd-extract/*")
  list(LENGTH _tops _top_count)
  if(_top_count EQUAL 1)
    list(GET _tops 0 _srcdir)
    file(GLOB _items "${_srcdir}/*")
    foreach(_item IN LISTS _items)
      get_filename_component(_name "${_item}" NAME)
      if(EXISTS "${ROOT}/${_name}")
        file(REMOVE_RECURSE "${ROOT}/${_name}")
      endif()
      file(RENAME "${_item}" "${ROOT}/${_name}")
    endforeach()
    file(REMOVE_RECURSE "${_srcdir}")
  endif()
  file(REMOVE_RECURSE "${ROOT}/.nfd-extract")
endfunction()

function(_argus_nfd_setup_wayland_protocols ROOT)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT ARGUS_GUI_NFD_WAYLAND)
    return()
  endif()

  set(_search_dirs "")
  if(ARGUS_WAYLAND_PROTOCOLS_DIR)
    list(APPEND _search_dirs "${ARGUS_WAYLAND_PROTOCOLS_DIR}")
  endif()

  argus_setup_wayland_protocols("${ROOT}/3ps/wayland-protocols"
      USE_SYSTEM "${ARGUS_GUI_NFD_USE_SYSTEM_WAYLAND_PROTOCOLS}"
      SEARCH_DIRS ${_search_dirs}
  )
endfunction()

function(_argus_nfd_ensure_sources ROOT)
  _argus_nfd_migrate_legacy_cache("${ROOT}")
  _argus_nfd_download_archive("${ROOT}")
  _argus_nfd_setup_wayland_protocols("${ROOT}")
endfunction()

#[[
    argus_setup_nfd()

    Fetches nativefiledialog-extended with CPM and builds the static nfd
    target. Tests are off.
]]
function(argus_setup_nfd)
  if(TARGET nfd)
    return()
  endif()

  _argus_nfd_cache_root(_nfd_root)
  _argus_nfd_ensure_sources("${_nfd_root}")

  set(_nfd_options
      "BUILD_SHARED_LIBS OFF"
      "NFD_BUILD_TESTS OFF"
      "NFD_BUILD_SDL2_TESTS OFF"
      "NFD_BUILD_SDL3_TESTS OFF"
      "NFD_BUILD_GLFW3_TESTS OFF"
      "NFD_INSTALL OFF"
  )
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND _nfd_options
        "NFD_WAYLAND ${ARGUS_GUI_NFD_WAYLAND}"
        "NFD_X11 ${ARGUS_GUI_NFD_X11}"
    )
    message(STATUS
        "nfd Linux: Wayland=${ARGUS_GUI_NFD_WAYLAND} X11=${ARGUS_GUI_NFD_X11}")
  endif()

  CPMAddPackage(
      NAME nfd
      VERSION ${ARGUS_NFD_VERSION}
      SOURCE_DIR "${_nfd_root}"
      EXCLUDE_FROM_ALL YES
      SYSTEM YES
      OPTIONS ${_nfd_options}
  )

  if(NOT TARGET nfd)
    message(FATAL_ERROR "nfd: CPM fetch did not provide the nfd target")
  endif()

  set_target_properties(nfd PROPERTIES FOLDER "third-party")
  argus_register_lib_target(nfd nfd)
  message(STATUS "nfd: ${ARGUS_NFD_VERSION} (${_nfd_root})")
endfunction()
