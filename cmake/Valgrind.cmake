function(AddValgrind target)
  if(EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    add_custom_target(valgrind
      COMMAND ${CMAKE_COMMAND} -E echo "Valgrind is not supported for Emscripten/WebAssembly test targets."
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    return()
  endif()

  find_program(VALGRIND_PATH valgrind REQUIRED)
  add_custom_target(valgrind
    COMMAND ${VALGRIND_PATH} --leak-check=yes --tool=memcheck
            $<TARGET_FILE:${target}> "~[slow]"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endfunction()
