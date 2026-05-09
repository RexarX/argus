set(ARGUS_CAPI_TEST_SOURCES
    main.c
)

add_executable(argus_bindings_capi_integration_tests ${ARGUS_CAPI_TEST_SOURCES})

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_c_standard(argus_bindings_capi_integration_tests STANDARD 17)
argus_target_set_platform(argus_bindings_capi_integration_tests)
argus_target_set_output_dirs(argus_bindings_capi_integration_tests CUSTOM_FOLDER tests MODULE bindings/capi)
argus_target_set_folder(argus_bindings_capi_integration_tests "tests/bindings/capi")
argus_target_set_optimization(argus_bindings_capi_integration_tests)

target_include_directories(argus_bindings_capi_integration_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/bindings/capi/include
)

target_link_libraries(argus_bindings_capi_integration_tests PRIVATE
    argus::bindings::capi
)

argus_copy_runtime_dependencies(argus_bindings_capi_integration_tests
    LIBRARIES argus::bindings::capi
)

add_test(
    NAME argus_bindings_capi_integration_tests
    COMMAND argus_bindings_capi_integration_tests
)

set_tests_properties(argus_bindings_capi_integration_tests PROPERTIES
    LABELS "bindings;capi"
)
