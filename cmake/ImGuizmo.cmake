# ImGuizmo 1.10, compiled from the release archive.
#
# https://github.com/CedricGuillemet/ImGuizmo/releases/tag/1.10

include_guard(GLOBAL)

set(ARGUS_IMGUIZMO_VERSION "1.10")

#[[
    argus_setup_imguizmo()

    Downloads the v1.10 archive and builds the static imguizmo target from
    ImGuizmo.cpp. Requires argus_setup_imgui().
]]
function(argus_setup_imguizmo)
  if(TARGET imguizmo)
    return()
  endif()
  if(NOT TARGET argus::lib::imgui)
    message(FATAL_ERROR "argus_setup_imguizmo: call argus_setup_imgui() first")
  endif()
  if(NOT CPM_SOURCE_CACHE)
    message(FATAL_ERROR "argus_setup_imguizmo: CPM_SOURCE_CACHE is not set")
  endif()

  set(_asset "ImGuizmo-${ARGUS_IMGUIZMO_VERSION}.zip")
  set(_hash
      "09e431b0a87a4c3c1910b151139c9fe3d4c175feca135ff4e3ef30ed0c1f6e20")
  set(_cache "${CPM_SOURCE_CACHE}/imguizmo")
  set(_archive "${_cache}/${_asset}")
  set(_root "${_cache}/ImGuizmo-${ARGUS_IMGUIZMO_VERSION}")
  set(_url
      "https://github.com/CedricGuillemet/ImGuizmo/archive/refs/tags/${ARGUS_IMGUIZMO_VERSION}.zip"
  )

  if(EXISTS "${_archive}")
    file(SHA256 "${_archive}" _actual)
    if(NOT _actual STREQUAL "${_hash}")
      message(STATUS "ImGuizmo: checksum mismatch, downloading ${_asset} again")
      file(REMOVE "${_archive}")
    endif()
  endif()

  if(NOT EXISTS "${_archive}")
    message(STATUS "ImGuizmo: downloading ${_asset}")
    file(MAKE_DIRECTORY "${_cache}")
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
      message(FATAL_ERROR "ImGuizmo download failed (${_code}): ${_message}")
    endif()
  endif()

  if(NOT EXISTS "${_root}/src/ImGuizmo.h")
    message(STATUS "ImGuizmo: extracting ${_asset}")
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_cache}")
  endif()

  if(NOT EXISTS "${_root}/src/ImGuizmo.cpp")
    message(FATAL_ERROR "ImGuizmo package is missing ${_root}/src/ImGuizmo.cpp")
  endif()

  add_library(imguizmo STATIC "${_root}/src/ImGuizmo.cpp")
  set_target_properties(imguizmo PROPERTIES
      FOLDER "third-party"
      POSITION_INDEPENDENT_CODE ON
  )
  target_include_directories(imguizmo SYSTEM PUBLIC "${_root}/src")
  target_link_libraries(imguizmo PRIVATE argus::lib::imgui)

  argus_register_lib_target(imguizmo imguizmo)
  message(STATUS "ImGuizmo: ${ARGUS_IMGUIZMO_VERSION} (${_root})")
endfunction()
