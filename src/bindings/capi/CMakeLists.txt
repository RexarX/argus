set(MODULE_NAME "BINDINGS_CAPI")

set(ARGUS_BINDINGS_CAPI_HEADERS
)

set(ARGUS_BINDINGS_CAPI_SOURCES
    src/library.cpp
)

set(ARGUS_BINDINGS_CAPI_ALL_FILES
    ${ARGUS_BINDINGS_CAPI_HEADERS}
    ${ARGUS_BINDINGS_CAPI_SOURCES}
)

add_library(argus_bindings_capi SHARED ${ARGUS_BINDINGS_CAPI_ALL_FILES})
add_library(argus::bindings::capi ALIAS argus_bindings_capi)

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_bindings_capi STANDARD 23)
argus_target_set_platform(argus_bindings_capi)
argus_target_set_shared_lib_platform(argus_bindings_capi)
argus_target_set_output_dirs(argus_bindings_capi MODULE bindings/capi)
argus_target_set_folder(argus_bindings_capi "argus/bindings/capi")

argus_target_add_pch(argus_bindings_capi ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp)
argus_target_set_warnings(argus_bindings_capi)
argus_target_set_optimization(argus_bindings_capi)
argus_target_enable_lto(argus_bindings_capi)
argus_target_enable_sanitizers(argus_bindings_capi)

set_target_properties(argus_bindings_capi PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_compile_definitions(argus_bindings_capi PRIVATE
  ARGUS_BUILD_SHARED
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(argus_bindings_capi PRIVATE -fvisibility=hidden)
endif()

target_include_directories(argus_bindings_capi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(argus_bindings_capi PRIVATE
    argus::core
    argus::common
    argus::algo
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
