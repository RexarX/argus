# ImPlot 1.0, compiled from the release archive.
#
# https://github.com/epezent/implot/releases/tag/v1.0

include_guard(GLOBAL)

set(ARGUS_IMPLOT_VERSION "1.0")

#[[
    argus_setup_implot()

    Downloads the v1.0 archive and builds the static implot target.
    Requires argus_setup_imgui().
]]
function(argus_setup_implot)
  if(TARGET implot)
    return()
  endif()
  if(NOT TARGET argus::lib::imgui)
    message(FATAL_ERROR "argus_setup_implot: call argus_setup_imgui() first")
  endif()
  if(NOT CPM_SOURCE_CACHE)
    message(FATAL_ERROR "argus_setup_implot: CPM_SOURCE_CACHE is not set")
  endif()

  set(_asset "implot-${ARGUS_IMPLOT_VERSION}.zip")
  set(_hash
      "c9e697db6eaafe5e944eeaae011ace1594447ea1d306997ebd4cc16f87caef30")
  set(_cache "${CPM_SOURCE_CACHE}/implot")
  set(_archive "${_cache}/${_asset}")
  set(_root "${_cache}/implot-${ARGUS_IMPLOT_VERSION}")
  set(_url
      "https://github.com/epezent/implot/archive/refs/tags/v${ARGUS_IMPLOT_VERSION}.zip"
  )

  if(EXISTS "${_archive}")
    file(SHA256 "${_archive}" _actual)
    if(NOT _actual STREQUAL "${_hash}")
      message(STATUS "ImPlot: checksum mismatch, downloading ${_asset} again")
      file(REMOVE "${_archive}")
    endif()
  endif()

  if(NOT EXISTS "${_archive}")
    message(STATUS "ImPlot: downloading ${_asset}")
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
      message(FATAL_ERROR "ImPlot download failed (${_code}): ${_message}")
    endif()
  endif()

  if(NOT EXISTS "${_root}/implot.h")
    message(STATUS "ImPlot: extracting ${_asset}")
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_cache}")
  endif()

  if(NOT EXISTS "${_root}/implot.cpp")
    message(FATAL_ERROR "ImPlot package is missing ${_root}/implot.cpp")
  endif()

  add_library(implot STATIC
      "${_root}/implot.cpp"
      "${_root}/implot_items.cpp"
      "${_root}/implot_demo.cpp"
  )
  set_target_properties(implot PROPERTIES
      FOLDER "third-party"
      POSITION_INDEPENDENT_CODE ON
  )
  target_include_directories(implot SYSTEM PUBLIC "${_root}")
  target_link_libraries(implot PRIVATE argus::lib::imgui)

  argus_register_lib_target(implot implot)
  message(STATUS "ImPlot: ${ARGUS_IMPLOT_VERSION} (${_root})")
endfunction()
