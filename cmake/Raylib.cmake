# Official raylib 6.2 static binaries.
#
# https://github.com/raysan5/raylib/releases/tag/6.2
# The shared library in each archive is not used.

include_guard(GLOBAL)

set(ARGUS_RAYLIB_VERSION "6.2")

function(_argus_raylib_asset OUT_NAME OUT_HASH OUT_LIB)
  if(WIN32)
    if(NOT MSVC AND NOT (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
      message(FATAL_ERROR
          "raylib's Windows binaries are MSVC static libraries. "
          "Configure with the MSVC or clang-cl preset."
      )
    endif()
    set(_lib "lib/raylib.lib")
    if(BIN_ARCHITECTURE STREQUAL "x64")
      set(${OUT_NAME} "raylib-${ARGUS_RAYLIB_VERSION}_win64_msvc16.zip" PARENT_SCOPE)
      set(${OUT_HASH}
          "8467b5dd6db4d7ab942f3148a3d30d93be6ed9b2fab88069d177e59604268ebd"
          PARENT_SCOPE
      )
    elseif(BIN_ARCHITECTURE STREQUAL "arm64")
      set(${OUT_NAME} "raylib-${ARGUS_RAYLIB_VERSION}_winarm64_msvc16.zip" PARENT_SCOPE)
      set(${OUT_HASH}
          "027454a16018793c173fe20005100487c246235e61d2a096ec2e31357bad4ac6"
          PARENT_SCOPE
      )
    elseif(BIN_ARCHITECTURE STREQUAL "x86")
      set(${OUT_NAME} "raylib-${ARGUS_RAYLIB_VERSION}_win32_msvc16.zip" PARENT_SCOPE)
      set(${OUT_HASH}
          "d5f1084ab81da3a9baa91eb3bf8f772faf96f8b0446a6ba994e297a3939cf485"
          PARENT_SCOPE
      )
    else()
      message(FATAL_ERROR
          "raylib ${ARGUS_RAYLIB_VERSION} has no Windows binary for "
          "${BIN_ARCHITECTURE}."
      )
    endif()
    set(${OUT_LIB} "${_lib}" PARENT_SCOPE)
    return()
  endif()

  if(APPLE)
    set(${OUT_NAME} "raylib-${ARGUS_RAYLIB_VERSION}_macos.tar.gz" PARENT_SCOPE)
    set(${OUT_HASH}
        "100437734a44d56ed9d2a1c4b249dbfc6d6cc3477a45e306f9f56fe5f1e2b4ef"
        PARENT_SCOPE
    )
    set(${OUT_LIB} "lib/libraylib.a" PARENT_SCOPE)
    return()
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_lib "lib/libraylib.a")
    if(BIN_ARCHITECTURE STREQUAL "x64")
      set(${OUT_NAME} "raylib-${ARGUS_RAYLIB_VERSION}_linux_amd64.tar.gz" PARENT_SCOPE)
      set(${OUT_HASH}
          "586bbc32f3596d439382b1c5dbc6c1c573eb7e1d00fe92be7d4deb116883720d"
          PARENT_SCOPE
      )
    elseif(BIN_ARCHITECTURE STREQUAL "arm64")
      set(${OUT_NAME} "raylib-${ARGUS_RAYLIB_VERSION}_linux_arm64.tar.gz" PARENT_SCOPE)
      set(${OUT_HASH}
          "281f3456c9e11c090442f5c1ca923c2c272c8fe53e6693fcd22f07c52f76da5a"
          PARENT_SCOPE
      )
    elseif(BIN_ARCHITECTURE STREQUAL "x86")
      set(${OUT_NAME} "raylib-${ARGUS_RAYLIB_VERSION}_linux_i386.tar.gz" PARENT_SCOPE)
      set(${OUT_HASH}
          "f9de4b5abbd2db1e3acf1844e942547ceb66406607b214314747975f867099c7"
          PARENT_SCOPE
      )
    else()
      message(FATAL_ERROR
          "raylib ${ARGUS_RAYLIB_VERSION} has no Linux binary for "
          "${BIN_ARCHITECTURE}."
      )
    endif()
    set(${OUT_LIB} "${_lib}" PARENT_SCOPE)
    return()
  endif()

  message(FATAL_ERROR
      "raylib ${ARGUS_RAYLIB_VERSION} has no binary for ${CMAKE_SYSTEM_NAME}."
  )
endfunction()

function(_argus_raylib_root_name ASSET OUT_VAR)
  if(ASSET MATCHES "\\.tar\\.gz$")
    string(LENGTH "${ASSET}" _length)
    math(EXPR _keep "${_length} - 7")
    string(SUBSTRING "${ASSET}" 0 ${_keep} _name)
  else()
    string(REGEX REPLACE "\\.zip$" "" _name "${ASSET}")
  endif()
  set(${OUT_VAR} "${_name}" PARENT_SCOPE)
endfunction()

function(_argus_raylib_system_libs OUT_VAR)
  if(WIN32)
    set(${OUT_VAR} "winmm;gdi32;opengl32;shell32;user32;ole32;uuid;advapi32" PARENT_SCOPE)
    return()
  endif()
  if(APPLE)
    set(${OUT_VAR}
        "-framework Cocoa;-framework IOKit;-framework CoreVideo;-framework OpenGL"
        PARENT_SCOPE)
    return()
  endif()
  set(${OUT_VAR}
      "GL;m;pthread;dl;rt;X11;Xrandr;Xi;Xinerama;Xcursor;Xrender;Xext"
      PARENT_SCOPE)
endfunction()

#[[
    argus_setup_raylib()

    Downloads the v6.2 archive into CPM_SOURCE_CACHE and imports the static
    raylib target. The DLL and shared objects in the archive are ignored.
    The Windows archive is the release CRT (/MD).
]]
function(argus_setup_raylib)
  if(TARGET raylib)
    return()
  endif()

  if(NOT CPM_SOURCE_CACHE)
    message(FATAL_ERROR "argus_setup_raylib: CPM_SOURCE_CACHE is not set")
  endif()

  _argus_raylib_asset(_asset _hash _lib)
  _argus_raylib_root_name("${_asset}" _root_name)

  set(_cache "${CPM_SOURCE_CACHE}/raylib")
  set(_archive "${_cache}/${_asset}")
  set(_root "${_cache}/${_root_name}")
  set(_library "${_root}/${_lib}")
  set(_include "${_root}/include")
  set(_url
      "https://github.com/raysan5/raylib/releases/download/${ARGUS_RAYLIB_VERSION}/${_asset}"
  )

  if(EXISTS "${_archive}")
    file(SHA256 "${_archive}" _actual)
    if(NOT _actual STREQUAL "${_hash}")
      message(STATUS "raylib: checksum mismatch, downloading ${_asset} again")
      file(REMOVE "${_archive}")
    endif()
  endif()

  if(NOT EXISTS "${_archive}")
    message(STATUS "raylib: downloading ${_asset}")
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
      message(FATAL_ERROR "raylib download failed (${_code}): ${_message}")
    endif()
  endif()

  if(NOT EXISTS "${_library}")
    message(STATUS "raylib: extracting ${_asset}")
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_cache}")
  endif()

  if(NOT EXISTS "${_library}")
    message(FATAL_ERROR "raylib package is missing ${_library}")
  endif()
  if(NOT EXISTS "${_include}/raylib.h")
    message(FATAL_ERROR "raylib package is missing ${_include}/raylib.h")
  endif()

  _argus_raylib_system_libs(_system)
  add_library(raylib STATIC IMPORTED GLOBAL)
  set_target_properties(raylib PROPERTIES
      IMPORTED_LOCATION "${_library}"
      INTERFACE_INCLUDE_DIRECTORIES "${_include}"
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_include}"
      INTERFACE_LINK_LIBRARIES "${_system}"
      FOLDER "third-party"
  )

  argus_register_lib_target(raylib raylib)
  message(STATUS "raylib: ${ARGUS_RAYLIB_VERSION} (${_library})")
endfunction()
