![Release screenshot](examples/Voxels/textures/maze_in_green_32x32.bmp)

# Maze Builder

Create and customize mazes on multiple platforms and languages. Below shows an example of the output string generated from the command-line interface: `mazebuildercli.exe -r 10 -c 10 -a dfs`

```text
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
|                 |                       |                 |
+-----+-----+     +     +-----+-----+     +     +     +     +
|                 |                 |     |     |     |     |
+     +-----+-----+     +-----+-----+     +     +     +-----+
|     |           |     |           |           |           |
+     +     +     +     +     +     +-----+     +-----+     +
|     |     |           |     |           |           |     |
+     +     +-----+-----+     +-----+     +-----+-----+     +
|           |           |     |           |                 |
+     +-----+     +     +-----+     +-----+     +-----+-----+
|     |           |           |     |                       |
+     +-----+-----+-----+     +     +-----+-----+-----+     +
|                       |     |     |                       |
+-----+-----+-----+     +     +     +     +-----+-----+     +
|                 |     |     |     |           |           |
+-----+-----+     +     +     +     +     +     +     +-----+
|                 |     |     |     |     |     |     |     |
+     +-----+-----+     +     +     +-----+     +     +     +
|                             |                 |           |
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
```

---

## Output Formats

The library provides multiple output formats like Wavefront object, PNG and JPEG images, JSON, and plain text or stdout.

Here's an example of the command-line program producing JSON output: `mazebuildercli.exe -r 2 -c 5 -o 2x5.json`

```json
{
  "rows": 2,
  "columns": 5,
  "seed": 2,
  "algo": "dfs",
  "distances": false,
  "str": "+---+---+---+---+---+\n
          |           |       |\n
          +   +---+   +---+   +\n
          |       |           |\n
          +---+---+---+---+---+\n"
}
```

---

## Help Message

View the help message from the command-line example: `./mazebuildercli --help`

```sh
        Usages: app.exe [OPTION(S)]... [OUTPUT]
        Generates mazes and exports to different formats
        Options: case-sensitive, long options must use '=' combination
        Example: app.exe -r 10 -c 10 -a binary_tree > out_maze.txt
        Example: app.exe --rows=10 --columns=10 --algo=dfs -o out_maze.txt
          -a, --algo         dfs, sidewinder, binary_tree [default]
          -c, --columns      columns
          -d, --distances    show distances using base36 numbers
          -e, --encode       encode maze to base64 string
          -h, --help         display this help message
          -j, --json         run with arguments in JSON format
          -s, --seed         seed for the mt19937 generator
          -r, --rows         rows
          -o, --output       [txt|text] [json] [jpg|jpeg] [png] 
                              [obj|object] [stdout]
          -v, --version      display program version
```

---

## Examples

Run the `binary_tree` algorithm with long arguments:

```sh
mazebuildercli.exe --rows=25 --columns=25 --seed=42 --algo=binary_tree --output=bt.obj
```

Run the `dfs` algorithm with short arguments:

```sh
mazebuildercli.exe -r 25 -c 25 -s 42 -a dfs -o dfs.obj
```

Use the C++ API in a modern C++ program:

```cpp
    #include <iostream>
    
    #include <MazeBuilder/maze_builder.h>

    void main() {
        auto rows{10}, cols{10};

        auto m = mazes::factory::create(rows, cols);

        auto s = mazes::stringz::stringify(m);

        std::cout << s << std::endl;

        return 0;
    }
```

---

## CMake Configuration and Testing

[CMake 3.2](https://cmake.org) or greater is required.
The examples and tests require external dependencies which can be grabbed from the Internet:
  - box2d
  - catch2
  - SDL
  - SFML

Use the following CMake options to configure the project:


| CMake Option | Default | Description |
|--------------|---------|------------ |
| MAZE_BUILDER_EXAMPLES | OFF | Build with project examples enabled. |
| MAZE_BUILDER_COVERAGE | OFF | Build with code coverage using `CppCheck`. |
| MAZE_BUILDER_TESTS | OFF | Build with testing using `Catch2`. |
| MAZE_BUILDER_DOCS | OFF | Build the docs using `doxygen`. |
| MAZE_BUILDER_MEMCHECK | OFF | Build with `Valgrind` and `Memcheck` support. |
| CMAKE_TOOLCHAIN_FILE | `cmake` | Building with a specific toolchain. Useful for [`Emscripten`](#cmake-web-builds) builds. |

---

### Build Commands

Configure it: `cmake -GNinja -S . -B build-examples -DMAZE_BUILDER_EXAMPLES:BOOL=ON`

Build it: `cmake --build build-examples --config Release`

By default, both a shared-object library and static library are produced.
The shared and static files have different naming conventions depending on the platform:

| Platform | static lib | shared lib |
| -------- | ---- | ---- |
| Windows | `mazebuildercore_static.lib` | `mazebuildercore_shared.dll` |
| Linux | `libmazebuildercore_static.a` | `libmazebuildercore_shared.so` |
| MacOS | | |

---

### Testing

Configure the project for testing: 
`cmake -S . -B build-tests -DMAZE_BUILDER_TESTS:BOOL=ON`

Run the tests: `ctest --test-dir build-tests/tests --verbose -C Debug`

---

### Configure for the Web

Configure examples for the browser using [Emscripten](https://emscripten.org/) and their toolchain file.

```sh
cmake -S . -B build-web -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${my/emsdk/repo}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
```

---

## Scripts

The Ruby script `mazes.rb` generates PNG images of mazes using algorithms and methods similar to the C++ library.
It is a good place to start learning about the maze-generating algorithms.
The Python script `solver.py` plays with the maze generation by loading PNG files and finding paths and networks.

**Script dependencies:**
  - `gem install chunky_png`
  - `pip install numpy pillow networkx`

---

## Web Interface

![](https://media1.giphy.com/media/v1.Y2lkPTc5MGI3NjExbjlnbjl6NmZ3c3hmMW05MDV1YXg1NjFuOW5ydHRlYW5xdjVvY3BsMCZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/iO02l5jhramJ43olgE/giphy.gif)

Provided is a web interface in a voxel world that enables interactive maze generation.

[Check out the this example in a live app!](https://jade-semifreddo-f24ef0.netlify.app/)

The web app can be run locally with the provided [secure_http_server.py](scripts/secure_http_server.py) script.
Once the provided script is running, then open the browser to `http://localhost:8000`.

## More Learning Resources

 - [box2d](https://box2d.org/documentation/hello.html)
 - [catch2](https://github.com/catchorg/Catch2)
 - [craft](https://github.com/fogleman/Craft)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [this README and these docs](https://zmertens.github.io/mazebuilder.github.io/index.html)
 - [Mazes for Programmers Book](https://www.jamisbuck.org/mazes/)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [SFML](https://github.com/SFML/SFML)
