# Fetch or locate wayland-protocols XML (e.g. for wayland-scanner at configure time).

include_guard(GLOBAL)

set(ARGUS_WAYLAND_PROTOCOLS_DEFAULT_COMMIT
    "88223018d1b578d0d8869866da66d9608e05f928"
)

function(argus_wayland_protocols_default_search_dirs OUT_LIST)
  set(_dirs "")
  if(DEFINED ARGUS_WAYLAND_PROTOCOLS_DIR AND ARGUS_WAYLAND_PROTOCOLS_DIR)
    list(APPEND _dirs "${ARGUS_WAYLAND_PROTOCOLS_DIR}")
  endif()
  if(DEFINED ENV{WAYLAND_PROTOCOLS_DIR})
    list(APPEND _dirs "$ENV{WAYLAND_PROTOCOLS_DIR}")
  endif()
  list(APPEND _dirs
      /usr/share/wayland-protocols
      /usr/local/share/wayland-protocols
  )
  set(${OUT_LIST} "${_dirs}" PARENT_SCOPE)
endfunction()

function(argus_wayland_protocols_xdg_foreign_marker PROTOCOLS_ROOT OUT_MARKER)
  set(${OUT_MARKER}
      "${PROTOCOLS_ROOT}/unstable/xdg-foreign/xdg-foreign-unstable-v1.xml"
      PARENT_SCOPE)
endfunction()

#[[
  argus_find_wayland_protocols(OUT_DIR [SEARCH_DIRS <dir>...])

  Sets OUT_DIR to a directory tree containing
  unstable/xdg-foreign/xdg-foreign-unstable-v1.xml, or empty if not found.
]]
function(argus_find_wayland_protocols OUT_DIR)
  set(_options "")
  set(_oneValueArgs "")
  set(_multiValueArgs SEARCH_DIRS)
  cmake_parse_arguments(_arg "" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

  set(_candidates "")
  if(_arg_SEARCH_DIRS)
    list(APPEND _candidates ${_arg_SEARCH_DIRS})
  endif()
  argus_wayland_protocols_default_search_dirs(_defaults)
  list(APPEND _candidates ${_defaults})

  foreach(_dir IN LISTS _candidates)
    argus_wayland_protocols_xdg_foreign_marker("${_dir}" _marker)
    if(_dir AND EXISTS "${_marker}")
      set(${OUT_DIR} "${_dir}" PARENT_SCOPE)
      return()
    endif()
  endforeach()
  set(${OUT_DIR} "" PARENT_SCOPE)
endfunction()

function(_argus_wayland_protocols_bundle_url COMMIT OUT_URL)
  set(${OUT_URL}
      "https://gitlab.freedesktop.org/wayland/wayland-protocols/-/archive/${COMMIT}/wayland-protocols-${COMMIT}.tar.gz"
      PARENT_SCOPE)
endfunction()

function(_argus_wayland_protocols_shared_cache_dir COMMIT OUT_VAR)
  if(NOT CPM_SOURCE_CACHE)
    set(${OUT_VAR} "" PARENT_SCOPE)
    return()
  endif()
  set(${OUT_VAR} "${CPM_SOURCE_CACHE}/wayland-protocols/${COMMIT}" PARENT_SCOPE)
endfunction()

#[[
  argus_setup_wayland_protocols(DESTINATION
      [COMMIT <git-commit>]
      [USE_SYSTEM <ON|OFF>]
      [SEARCH_DIRS <dir>...]
  )

  Ensures unstable/xdg-foreign/xdg-foreign-unstable-v1.xml exists under DESTINATION.
]]
function(argus_setup_wayland_protocols DESTINATION)
  set(_options "")
  set(_oneValueArgs COMMIT USE_SYSTEM)
  set(_multiValueArgs SEARCH_DIRS)
  cmake_parse_arguments(_arg "" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

  if(NOT _arg_COMMIT)
    set(_arg_COMMIT "${ARGUS_WAYLAND_PROTOCOLS_DEFAULT_COMMIT}")
  endif()
  if(NOT _arg_USE_SYSTEM)
    set(_arg_USE_SYSTEM ON)
  endif()

  get_filename_component(_dest "${DESTINATION}" ABSOLUTE)
  argus_wayland_protocols_xdg_foreign_marker("${_dest}" _marker)
  if(EXISTS "${_marker}")
    return()
  endif()

  get_filename_component(_dest_parent "${_dest}" DIRECTORY)
  file(MAKE_DIRECTORY "${_dest_parent}")
  if(EXISTS "${_dest}")
    file(REMOVE_RECURSE "${_dest}")
  endif()

  if(_arg_USE_SYSTEM)
    argus_find_wayland_protocols(_system SEARCH_DIRS ${_arg_SEARCH_DIRS})
    if(_system)
      message(STATUS "wayland-protocols: using system tree at ${_system}")
      file(CREATE_LINK "${_system}" "${_dest}" SYMBOLIC)
      if(EXISTS "${_marker}")
        return()
      endif()
      file(REMOVE_RECURSE "${_dest}")
      message(WARNING
          "wayland-protocols: system symlink failed; falling back to bundled copy")
    else()
      message(STATUS
          "wayland-protocols: system tree not found; downloading bundled copy")
    endif()
  endif()

  _argus_wayland_protocols_shared_cache_dir("${_arg_COMMIT}" _shared)
  argus_wayland_protocols_xdg_foreign_marker("${_shared}" _shared_marker)

  if(_shared AND EXISTS "${_shared_marker}")
    file(COPY "${_shared}/" DESTINATION "${_dest}")
  else()
    _argus_wayland_protocols_bundle_url("${_arg_COMMIT}" _url)
    message(STATUS "wayland-protocols: fetching ${_arg_COMMIT}")
    set(_archive "${_dest_parent}/.wayland-protocols.tar.gz")
    file(DOWNLOAD "${_url}" "${_archive}"
        EXPECTED_HASH
            "SHA256=55a45095c05b2062d9efd63ce5325634b6a7c8b56d25ee5ac98a50231962cfe0"
        SHOW_PROGRESS
        TLS_VERIFY ON
    )
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_dest_parent}")
    file(REMOVE "${_archive}")

    file(GLOB _extracted "${_dest_parent}/wayland-protocols-*")
    list(LENGTH _extracted _count)
    if(NOT _count EQUAL 1)
      message(FATAL_ERROR
          "wayland-protocols: unexpected archive layout under \"${_dest_parent}\"")
    endif()
    list(GET _extracted 0 _extracted_dir)
    file(RENAME "${_extracted_dir}" "${_dest}")

    if(_shared AND EXISTS "${_marker}")
      file(MAKE_DIRECTORY "${_shared}")
      file(COPY "${_dest}/" DESTINATION "${_shared}")
    endif()
  endif()

  if(NOT EXISTS "${_marker}")
    message(FATAL_ERROR
        "wayland-protocols: missing xdg-foreign protocol XML under \"${_dest}\"")
  endif()
endfunction()
