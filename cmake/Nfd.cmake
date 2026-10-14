# nativefiledialog-extended 1.4.1, static.
#
# https://github.com/btzy/nativefiledialog-extended/releases/tag/v1.4.1

include_guard(GLOBAL)
include(CPM)

set(ARGUS_NFD_VERSION "1.4.1")

#[[
    argus_setup_nfd()

    Fetches nativefiledialog-extended with CPM and builds the static nfd
    target. Tests are off.
]]
function(argus_setup_nfd)
  if(TARGET nfd)
    return()
  endif()

  CPMAddPackage(
      NAME nfd
      VERSION ${ARGUS_NFD_VERSION}
      URL "https://github.com/btzy/nativefiledialog-extended/archive/refs/tags/v${ARGUS_NFD_VERSION}.zip"
      URL_HASH "SHA256=ffd177323a36c313de0ef0187f956aa65f32f12639f86fb204b5c8558e0a22f6"
      EXCLUDE_FROM_ALL YES
      SYSTEM YES
      OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "NFD_BUILD_TESTS OFF"
        "NFD_BUILD_SDL2_TESTS OFF"
        "NFD_BUILD_SDL3_TESTS OFF"
        "NFD_BUILD_GLFW3_TESTS OFF"
        "NFD_INSTALL OFF"
  )

  if(NOT TARGET nfd)
    message(FATAL_ERROR "nfd: CPM fetch did not provide the nfd target")
  endif()

  set_target_properties(nfd PROPERTIES FOLDER "third-party")
  argus_register_lib_target(nfd nfd)
  message(STATUS "nfd: ${ARGUS_NFD_VERSION}")
endfunction()
