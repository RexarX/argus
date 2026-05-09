set(ARGUS_COMMON_TEST_SOURCES
    main.cpp
)

add_executable(argus_common_tests ${ARGUS_COMMON_TEST_SOURCES})

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_common_tests STANDARD 23)
argus_target_set_platform(argus_common_tests)
argus_target_set_output_dirs(argus_common_tests CUSTOM_FOLDER tests MODULE common)
argus_target_set_folder(argus_common_tests "argus/common/tests")

argus_target_set_warnings(argus_common_tests)
argus_target_set_optimization(argus_common_tests)
argus_target_enable_lto(argus_common_tests)
argus_target_enable_sanitizers(argus_common_tests)

argus_require_dependency(doctest)
target_link_libraries(argus_common_tests PRIVATE
    argus::common
    argus::doctest::doctest
)

add_test(NAME argus_common_tests COMMAND argus_common_tests)
