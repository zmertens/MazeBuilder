find_package(Doxygen REQUIRED dot)

macro(InitDoxygenAwesomeCss)
    # Need to get properties for target here
    FetchContent_Declare(
        doxygen-awesome-css
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG v2.3.1
    )
    FetchContent_MakeAvailable(doxygen-awesome-css)
    FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_FULL_SIDEBAR NO)
    set(DOXYGEN_DISABLE_INDEX NO)
    set(DOXYGEN_HAVE_DOT YES)
    set(DOXYGEN_DOT_IMAGE_FORMAT svg)
    set(DOXYGEN_DOT_TRANSPARENT YES)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET ${doxygen-awesome-css_SOURCE_DIR}/doxygen-awesome.css)

    message(INFO "${doxygen-awesome-css_SOURCE_DIR}")
    set(DOXYGEN_HTML_EXTRA_FILES
        ${doxygen-awesome-css_SOURCE_DIR}/doxygen-awesome-darkmode-toggle.js
        ${doxygen-awesome-css_SOURCE_DIR}/doxygen-awesome-fragment-copy-button.js
        ${doxygen-awesome-css_SOURCE_DIR}/doxygen-awesome-paragraph-link.js
        ${doxygen-awesome-css_SOURCE_DIR}/doxygen-awesome-interactive-toc.js
    )

    execute_process(COMMAND doxygen -w html header.html footer.html style.css
                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/docs/html)
    execute_process(COMMAND sed -i "/<\\/head>/r ${PROJECT_BINARY_DIR}/cmake/extra_headers.html.txt" header.html
                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/docs/html)
    set(DOXYGEN_HTML_HEADER ${PROJECT_BINARY_DIR}/docs/html/header.html)
endmacro()

set(DOXYGEN_PROJECT_NAME "Maze Builder")
set(DOXYGEN_GENERATE_HTML YES)
set(DOXYGEN_HTML_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html")
set(DOXYGEN_EXTRACT_ALL NO)
set(DOXYGEN_FILE_PATTERNS *.h)
set(DOXYGEN_ENABLE_PREPROCESSING YES)
set(DOXYGEN_MACRO_EXPANSION YES)
set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)

set(DOXYGEN_IMAGE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../examples/Voxels/textures")
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/overview.md")
set(DOXYGEN_PROJECT_LOGO "${CMAKE_CURRENT_SOURCE_DIR}/../examples/Voxels/textures/favicon.ico")

set(DOXYGEN_INLINE_SIMPLE_STRUCTS YES)
set(DOXYGEN_TYPEDEF_HIDES_STRUCT YES)

# this tells doxygen to label structs as structs instead of classes
set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C NO)
set(DOXYGEN_WARN_IF_INCOMPLETE_DOC NO)

InitDoxygenAwesomeCss()

doxygen_add_docs(mazebuilderdocs
                "${CMAKE_SOURCE_DIR}/include/MazeBuilder"
                "overview.md"
                ALL
                COMMENT "Generate HTML documentation")
