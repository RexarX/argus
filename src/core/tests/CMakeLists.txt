set(ARGUS_CORE_TEST_SOURCES
    main.cpp

    assert.cpp
    compiler.cpp
    platform.cpp
    stacktrace.cpp

    container/static_string.cpp

    utils/filesystem.cpp
    utils/hash.cpp
    utils/macro.cpp
    utils/type_info.cpp
)

add_executable(argus_core_tests ${ARGUS_CORE_TEST_SOURCES})

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_core_tests STANDARD 23)
argus_target_set_platform(argus_core_tests)
argus_target_set_output_dirs(argus_core_tests CUSTOM_FOLDER tests MODULE core)
argus_target_set_folder(argus_core_tests "argus/core/tests")

argus_target_set_warnings(argus_core_tests)
argus_target_set_optimization(argus_core_tests)
argus_target_enable_lto(argus_core_tests)
argus_target_enable_sanitizers(argus_core_tests)

argus_require_dependency(doctest)
target_link_libraries(argus_core_tests PRIVATE
    argus::core
    argus::doctest::doctest
)

add_test(NAME argus_core_tests COMMAND argus_core_tests)
