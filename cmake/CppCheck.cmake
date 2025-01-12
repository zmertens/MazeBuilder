function(AddCppCheck target)
  find_program(CPPCHECK_EXECUTABLE cppcheck REQUIRED)
  set_target_properties(${target}
    PROPERTIES CXX_CPPCHECK
    "${CPPCHECK_EXECUTABLE};--enable=warning;--error-exitcode=1"
  )
endfunction()
