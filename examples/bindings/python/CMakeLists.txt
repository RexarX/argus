if(NOT TARGET argus_python)
  message(FATAL_ERROR "basic_python_example requires argus_python (ARGUS_BUILD_PYTHON_BINDINGS=ON)")
endif()

add_custom_target(basic_python_example
    COMMAND ${CMAKE_COMMAND} -E env
        "PYTHONPATH=$<TARGET_FILE_DIR:argus_python>"
        "${Python_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/basic.py"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS argus_python
    COMMENT "Run basic Python example"
    VERBATIM
)

argus_target_set_folder(basic_python_example "examples")
