set(ARGUS_ALGO_TEST_SOURCES
    main.cpp
)

add_executable(argus_algo_tests ${ARGUS_ALGO_TEST_SOURCES})

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_algo_tests STANDARD 23)
argus_target_set_platform(argus_algo_tests)
argus_target_set_output_dirs(argus_algo_tests CUSTOM_FOLDER tests MODULE algo)
argus_target_set_folder(argus_algo_tests "argus/algo/tests")

argus_target_set_warnings(argus_algo_tests)
argus_target_set_optimization(argus_algo_tests)
argus_target_enable_lto(argus_algo_tests)
argus_target_enable_sanitizers(argus_algo_tests)

argus_require_dependency(doctest)
target_link_libraries(argus_algo_tests PRIVATE
    argus::algo
    argus::doctest::doctest
)

add_test(NAME argus_algo_tests COMMAND argus_algo_tests)
