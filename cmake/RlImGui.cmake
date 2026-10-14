# rlImGui for raylib 6.0, compiled from the release archive.
#
# https://github.com/raylib-extras/rlImGui/releases/tag/Raylib_6_0
# The upstream premake project is not used.

include_guard(GLOBAL)

set(ARGUS_RLIMGUI_TAG "Raylib_6_0")

#[[
    argus_setup_rlimgui()

    Downloads the Raylib_6_0 archive and builds the static rlimgui target.
    Requires argus_setup_imgui() and argus_setup_raylib().
]]
function(argus_setup_rlimgui)
  if(TARGET rlimgui)
    return()
  endif()
  if(NOT TARGET argus::lib::imgui OR NOT TARGET argus::lib::raylib)
    message(FATAL_ERROR
        "argus_setup_rlimgui: call argus_setup_imgui() and argus_setup_raylib() first"
    )
  endif()
  if(NOT CPM_SOURCE_CACHE)
    message(FATAL_ERROR "argus_setup_rlimgui: CPM_SOURCE_CACHE is not set")
  endif()

  set(_asset "rlImGui-${ARGUS_RLIMGUI_TAG}.zip")
  set(_hash
      "7081c41dfa01261ab2947cc41c5a2abffe4fab477ca3b686b3f201cf4a2f43ab")
  set(_cache "${CPM_SOURCE_CACHE}/rlimgui")
  set(_archive "${_cache}/${_asset}")
  set(_root "${_cache}/rlImGui-${ARGUS_RLIMGUI_TAG}")
  set(_url
      "https://github.com/raylib-extras/rlImGui/archive/refs/tags/${ARGUS_RLIMGUI_TAG}.zip"
  )

  if(EXISTS "${_archive}")
    file(SHA256 "${_archive}" _actual)
    if(NOT _actual STREQUAL "${_hash}")
      message(STATUS "rlImGui: checksum mismatch, downloading ${_asset} again")
      file(REMOVE "${_archive}")
    endif()
  endif()

  if(NOT EXISTS "${_archive}")
    message(STATUS "rlImGui: downloading ${_asset}")
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
      message(FATAL_ERROR "rlImGui download failed (${_code}): ${_message}")
    endif()
  endif()

  if(NOT EXISTS "${_root}/rlImGui.h")
    message(STATUS "rlImGui: extracting ${_asset}")
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_cache}")
  endif()

  if(NOT EXISTS "${_root}/rlImGui.cpp")
    message(FATAL_ERROR "rlImGui package is missing ${_root}/rlImGui.cpp")
  endif()

  add_library(rlimgui STATIC "${_root}/rlImGui.cpp")
  set_target_properties(rlimgui PROPERTIES
      FOLDER "third-party"
      POSITION_INDEPENDENT_CODE ON
  )
  target_include_directories(rlimgui SYSTEM PUBLIC "${_root}")
  target_link_libraries(rlimgui PRIVATE argus::lib::imgui argus::lib::raylib)

  argus_register_lib_target(rlimgui rlimgui)
  message(STATUS "rlImGui: ${ARGUS_RLIMGUI_TAG} (${_root})")
endfunction()
