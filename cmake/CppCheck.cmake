function(AddCppCheck target)
    # Optionally, set the path to cppcheck if provided
    if(DEFINED CPPCHECK_EXECUTABLE)
        message(STATUS "Using cppcheck exe: ${CPPCHECK_EXECUTABLE}")
    else()
        find_program(CPPCHECK_EXECUTABLE cppcheck)
        if(NOT CPPCHECK_EXECUTABLE)
            message(WARNING "cppcheck not found, static analysis will be skipped")
            return()
        endif()
    endif()

    set_target_properties(${target}
        PROPERTIES CXX_CPPCHECK
        "${CPPCHECK_EXECUTABLE};--enable=warning;--error-exitcode=1"
    )
endfunction()