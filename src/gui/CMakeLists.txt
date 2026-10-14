set(ARGUS_GUI_HEADERS
    include/argus/gui/app.hpp
)

set(ARGUS_GUI_SOURCES
    src/main.cpp
)

set(ARGUS_GUI_FILES
    ${ARGUS_GUI_HEADERS}
    ${ARGUS_GUI_SOURCES}
)

add_executable(argus_gui ${ARGUS_GUI_FILES})

argus_apply_conventions(argus_gui)
argus_target_set_output_dirs(argus_gui CUSTOM_FOLDER gui)
argus_target_set_folder(argus_gui "gui")
argus_target_add_pch(argus_gui "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp")

target_include_directories(argus_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(argus_gui PRIVATE
    argus::core
    argus::common
    argus::sim
    argus::lib::raylib
    argus::lib::rlimgui
    argus::lib::imgui
    argus::lib::implot
    argus::lib::imguizmo
    argus::lib::nfd
)

# The prebuilt raylib static library uses the release CRT (/MD). Debug
# convention flags compile /MDd, which cannot link that archive.
if(MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
    AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
  set_property(TARGET argus_gui PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
  )
  target_compile_options(argus_gui PRIVATE $<$<CONFIG:Debug>:/MD>)
endif()
