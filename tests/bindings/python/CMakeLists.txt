find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Copy Python module for testing
add_custom_target(argus_bindings_python_integration_tests_setup ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/tests/python
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:argus_bindings_python>
        ${CMAKE_BINARY_DIR}/tests/python
    COMMENT "Setting up Python test environment"
    DEPENDS argus_bindings_python
)

add_test(
    NAME argus_bindings_python_integration_tests
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/main.py
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_tests_properties(argus_bindings_python_integration_tests PROPERTIES
    ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/tests/python:$ENV{PYTHONPATH}"
    LABELS "bindings;python"
    DEPENDS argus_bindings_python_integration_tests_setup
)
