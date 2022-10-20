function(add_unit_test test_name test_libraries)
  string(APPEND file_name ${test_name} ".cc")
  string(JOIN "/" file_path "tests" ${file_name})
  string(APPEND target_name "test-" ${test_name})
  add_executable(${target_name} ${file_path})
  target_link_libraries(${target_name} PRIVATE ${test_libraries})
  add_test(NAME ${target_name} COMMAND $<TARGET_FILE:${target_name}>)
endfunction()