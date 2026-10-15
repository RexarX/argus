# Open Asset Import Library 6.0.5, static.
#
# https://github.com/assimp/assimp/releases/tag/v6.0.5

include_guard(GLOBAL)
include(CPM)

set(ARGUS_ASSIMP_VERSION "6.0.5")

#[[
    argus_setup_assimp()

    Fetches Assimp with CPM and builds the static assimp target. Importers
    only. Exceptions and RTTI are restored on that target because argus strips
    them from the global MSVC flags.
]]
function(argus_setup_assimp)
  if(TARGET assimp)
    return()
  endif()

  CPMAddPackage(
      NAME assimp
      VERSION ${ARGUS_ASSIMP_VERSION}
      URL "https://github.com/assimp/assimp/archive/refs/tags/v${ARGUS_ASSIMP_VERSION}.zip"
      URL_HASH "SHA256=89e477e89df739ecc7dd21874897ac2e70c8944eb1074fbdbbdbf4dd2de6a403"
      EXCLUDE_FROM_ALL YES
      SYSTEM YES
      OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "ASSIMP_BUILD_TESTS OFF"
        "ASSIMP_BUILD_ASSIMP_TOOLS OFF"
        "ASSIMP_BUILD_SAMPLES OFF"
        "ASSIMP_BUILD_DOCS OFF"
        "ASSIMP_INSTALL OFF"
        "ASSIMP_NO_EXPORT ON"
        "ASSIMP_WARNINGS_AS_ERRORS OFF"
        "ASSIMP_IGNORE_GIT_HASH ON"
        "USE_STATIC_CRT OFF"
  )

  if(NOT TARGET assimp)
    message(FATAL_ERROR "Assimp: CPM fetch did not provide the assimp target")
  endif()

  set_target_properties(assimp PROPERTIES FOLDER "third-party")
  if(WIN32)
    # Preprocessing assimp.rc pulls in Windows.h through every Assimp include
    # and stalls the resource compiler. The static library does not need it.
    set_source_files_properties("${assimp_SOURCE_DIR}/code/res/assimp.rc"
        TARGET_DIRECTORY assimp
        PROPERTIES HEADER_FILE_ONLY TRUE
    )
  endif()
  if(MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
      AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    target_compile_options(assimp PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
        $<$<COMPILE_LANGUAGE:CXX>:/GR>
    )
  endif()

  argus_register_lib_target(assimp assimp)
  if(TARGET zlibstatic)
    argus_register_lib_target(zlibstatic zlibstatic)
  endif()

  _argus_assimp_fix_linux_minizip_includes()

  message(STATUS "Assimp: ${ARGUS_ASSIMP_VERSION}")
endfunction()

function(_argus_assimp_fix_linux_minizip_includes)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT TARGET assimp)
    return()
  endif()

  # Assimp includes <unzip.h>, while many distros (e.g. Arch) install it as
  # <minizip/unzip.h> and only add /usr/include to the pkg-config cflags.
  find_package(PkgConfig QUIET)
  if(NOT PKG_CONFIG_FOUND)
    return()
  endif()

  pkg_check_modules(_argus_assimp_minizip minizip)
  if(NOT _argus_assimp_minizip_FOUND)
    return()
  endif()

  foreach(_dir IN LISTS _argus_assimp_minizip_INCLUDE_DIRS)
    if(EXISTS "${_dir}/minizip/unzip.h")
      target_include_directories(assimp SYSTEM PRIVATE "${_dir}/minizip")
    endif()
  endforeach()
endfunction()
