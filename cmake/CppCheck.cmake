function(AddCppCheck target)
    # Optionally, set the path to cppcheck if provided
    if(DEFINED CPPCHECK_EXECUTABLE)
        message(INFO ": Using cppcheck exe: ${CPPCHECK_EXECUTABLE}")
    else()
        find_program(CPPCHECK_EXECUTABLE cppcheck REQUIRED)
    endif()

    if(NOT CPPCHECK_EXECUTABLE)
        message(FATAL_ERROR ": ${CPPCHECK_EXECUTABLE} not found")
    else()
        set_target_properties(${target}
            PROPERTIES CXX_CPPCHECK
            "${CPPCHECK_EXECUTABLE};--enable=warning;--error-exitcode=1"
        )
    endif()
endfunction()
