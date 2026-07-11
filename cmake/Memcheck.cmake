function(AddMemcheck target)
  if(EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    add_custom_target(memcheck
      COMMAND ${CMAKE_COMMAND} -E echo "memcheck-cover is not supported for Emscripten/WebAssembly test targets."
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    return()
  endif()

  include(FetchContent)
  FetchContent_Declare(memcheckcover
    GIT_REPOSITORY https://github.com/Farigh/memcheck-cover.git
    GIT_TAG release-1.2
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(memcheckcover)
  FetchContent_GetProperties(memcheckcover SOURCE_DIR MEMCHECKCOVER_SOURCE_DIR)

  set(MEMCHECK_PATH "${MEMCHECKCOVER_SOURCE_DIR}/bin")
  if(MEMCHECKCOVER_SOURCE_DIR STREQUAL "" OR NOT EXISTS "${MEMCHECK_PATH}/memcheck_runner.sh")
    message(FATAL_ERROR "memcheck-cover scripts not found at ${MEMCHECK_PATH}")
  endif()

  add_custom_target(memcheck
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/valgrind"
    COMMAND ${MEMCHECK_PATH}/memcheck_runner.sh -o
            "${CMAKE_BINARY_DIR}/valgrind/report"
        -- $<TARGET_FILE:${target}> "~[slow]"
    COMMAND ${MEMCHECK_PATH}/generate_html_report.sh
            -i "${CMAKE_BINARY_DIR}/valgrind"
            -o "${CMAKE_BINARY_DIR}/valgrind"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endfunction()
