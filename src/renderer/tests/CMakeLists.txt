set(ARGUS_RENDERER_TEST_SOURCES
    main.cpp
)

add_executable(argus_renderer_tests ${ARGUS_RENDERER_TEST_SOURCES})

include(${PROJECT_SOURCE_DIR}/cmake/TargetUtils.cmake)

argus_target_set_cxx_standard(argus_renderer_tests STANDARD 23)
argus_target_set_platform(argus_renderer_tests)
argus_target_set_output_dirs(argus_renderer_tests CUSTOM_FOLDER tests MODULE renderer)
argus_target_set_folder(argus_renderer_tests "argus/renderer/tests")

argus_target_set_warnings(argus_renderer_tests)
argus_target_set_optimization(argus_renderer_tests)
argus_target_enable_lto(argus_renderer_tests)
argus_target_enable_sanitizers(argus_renderer_tests)

argus_require_dependency(doctest)
target_link_libraries(argus_renderer_tests PRIVATE
    argus::doctest::doctest
)

add_test(NAME argus_renderer_tests COMMAND argus_renderer_tests)
