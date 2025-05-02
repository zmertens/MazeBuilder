![Release screenshot](examples/Voxels/textures/maze_in_green_32x32.bmp)

# Maze Builder

Create and customize mazes on multiple platforms and languages.
An example of an output string generated from the command-line interface is shown here:

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

## Examples

### Command-Line Interface

The command-line interface example allows maze building functionality at the terminal.

Run the `binary_tree` algorithm with long arguments:

```sh
mazebuildercli.exe --rows=25 --columns=25 --seed=42 --algo=binary_tree --output=bt.obj
```

Run the `dfs` algorithm with short arguments:

```sh
mazebuildercli.exe -r 25 -c 25 -s 42 -a dfs -o dfs.obj
```

### API 

Use the C++ API in a modern C++ program:

```cpp
    #include <iostream>
    
    #include <MazeBuilder/maze_builder.h>

    void main() {
        auto rows{10}, cols{10};

        auto m = mazes::factory::create_q(rows, cols);

        auto s = mazes::stringz::stringify(m);

        std::cout << s << std::endl;

        return 0;
    }
```

### Web Interface - Voxels

![](https://media1.giphy.com/media/v1.Y2lkPTc5MGI3NjExbjlnbjl6NmZ3c3hmMW05MDV1YXg1NjFuOW5ydHRlYW5xdjVvY3BsMCZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/iO02l5jhramJ43olgE/giphy.gif)

Provided is a web interface in a voxel world that enables interactive maze generation.

[Check out the this example in a live app!](https://jade-semifreddo-f24ef0.netlify.app/)

The web app can be run locally with the provided [secure_http_server.py](scripts/secure_http_server.py) script.
Once the provided script is running, then open the browser to `http://localhost:8000`.

### Http

The HTTP example allows for distributed maze building and sharing.
The network endpoint is configurable with the included JSON input files.

### Physics

The physics example is a full-fledged video game. Pick balls and bounce against maze walls, breaking and colliding, and navigating to exit cells.

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

## More Learning Resources

 - [box2d](https://box2d.org/documentation/hello.html)
 - [catch2](https://github.com/catchorg/Catch2)
 - [craft](https://github.com/fogleman/Craft)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [this README and these docs](https://zmertens.github.io/mazebuilder.github.io/index.html)
 - [Mazes for Programmers Book](https://www.jamisbuck.org/mazes/)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [SFML](https://github.com/SFML/SFML)
