add_executable(basic_c_example basic.c)

argus_apply_conventions(basic_c_example)
argus_target_set_output_dirs(basic_c_example CUSTOM_FOLDER examples)
argus_target_set_folder(basic_c_example "examples")

target_link_libraries(basic_c_example PRIVATE argus::c)
if(ARGUS_ENABLE_EMBREE_BACKEND)
  argus_target_copy_embree_runtime(basic_c_example)
endif()
