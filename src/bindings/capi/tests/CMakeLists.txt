set(ARGUS_BINDINGS_CAPI_TEST_SOURCES
    main.cpp
)

add_executable(argus_bindings_capi_tests ${ARGUS_BINDINGS_CAPI_TEST_SOURCES})

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_bindings_capi_tests STANDARD 23)
argus_target_set_platform(argus_bindings_capi_tests)
argus_target_set_output_dirs(argus_bindings_capi_tests CUSTOM_FOLDER tests MODULE bindings/capi)
argus_target_set_folder(argus_bindings_capi_tests "argus/bindings/capi/tests")

argus_target_set_warnings(argus_bindings_capi_tests)
argus_target_set_optimization(argus_bindings_capi_tests)
argus_target_enable_lto(argus_bindings_capi_tests)
argus_target_enable_sanitizers(argus_bindings_capi_tests)

argus_require_dependency(doctest)
target_link_libraries(argus_bindings_capi_tests PRIVATE
    argus::bindings::capi
    argus::doctest::doctest
)

add_test(NAME argus_bindings_capi_tests COMMAND argus_bindings_capi_tests)
