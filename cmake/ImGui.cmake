# Dear ImGui v1.92.7 docking branch, compiled from the release archive.
#
# https://github.com/ocornut/imgui/archive/refs/tags/v1.92.7-docking.zip

include_guard(GLOBAL)

set(ARGUS_IMGUI_VERSION "1.92.7-docking")

#[[
    argus_setup_imgui()

    Downloads the docking archive and builds the static imgui target,
    including the demo sources.
]]
function(argus_setup_imgui)
  if(TARGET imgui)
    return()
  endif()

  if(NOT CPM_SOURCE_CACHE)
    message(FATAL_ERROR "argus_setup_imgui: CPM_SOURCE_CACHE is not set")
  endif()

  set(_asset "imgui-${ARGUS_IMGUI_VERSION}.zip")
  set(_hash
      "59a30664ea9dd80803f936ba9726c9ddbee431dfe1708e217da86bb069f251a8")
  set(_cache "${CPM_SOURCE_CACHE}/imgui")
  set(_archive "${_cache}/${_asset}")
  set(_root "${_cache}/imgui-${ARGUS_IMGUI_VERSION}")
  set(_url
      "https://github.com/ocornut/imgui/archive/refs/tags/v${ARGUS_IMGUI_VERSION}.zip"
  )

  if(EXISTS "${_archive}")
    file(SHA256 "${_archive}" _actual)
    if(NOT _actual STREQUAL "${_hash}")
      message(STATUS "ImGui: checksum mismatch, downloading ${_asset} again")
      file(REMOVE "${_archive}")
    endif()
  endif()

  if(NOT EXISTS "${_archive}")
    message(STATUS "ImGui: downloading ${_asset}")
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
      message(FATAL_ERROR "ImGui download failed (${_code}): ${_message}")
    endif()
  endif()

  if(NOT EXISTS "${_root}/imgui.h")
    message(STATUS "ImGui: extracting ${_asset}")
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_cache}")
  endif()

  if(NOT EXISTS "${_root}/imgui.h")
    message(FATAL_ERROR "ImGui package is missing ${_root}/imgui.h")
  endif()

  add_library(imgui STATIC
      "${_root}/imgui.cpp"
      "${_root}/imgui_draw.cpp"
      "${_root}/imgui_tables.cpp"
      "${_root}/imgui_widgets.cpp"
      "${_root}/imgui_demo.cpp"
  )
  set_target_properties(imgui PROPERTIES
      FOLDER "third-party"
      POSITION_INDEPENDENT_CODE ON
  )
  target_include_directories(imgui SYSTEM PUBLIC "${_root}")

  argus_register_lib_target(imgui imgui)
  message(STATUS "ImGui: ${ARGUS_IMGUI_VERSION} (${_root})")
endfunction()
