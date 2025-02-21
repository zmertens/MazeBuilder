![Release screenshot](examples/Voxels/textures/maze_in_green_32x32.bmp)

# Maze Builder

Build different representations of mazes quickly and easily on multiple platforms and languages. Below is an example of the string generated from the command-line interface.

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

### Exports

The library provides multiple export formats like Wavefront object, PNG and JPEG images, JSON, and plain text or `stdout`.
The library enables rapid prototyping and generating examples for games and simulations.
The exports can be integrated into game engines and renderers like `Unity`, `Godot`, `Blender`, `Unreal Engine` and so forth.

## Examples and the Help Message

```sh
        Usages: mazebuildercli.exe [OPTION(S)]... [OUTPUT]
          Program generates strings with multiple export formats
        Example: mazebuildercli.exe -r 10 -c 10 -a binary_tree -o out.txt
          -a, --algorithm    dfs, sidewinder, binary_tree
          -s, --seed         seed for the mt19937 generator
          -r, --rows         maze rows
          -l, --levels       maze levels (also known as height in 3D)
          -c, --columns      maze columns
          -d, --distances    show distances between cells as integers
          -o, --output       [.txt], [.png|.jpg], [.obj],
                                [.json], [stdout]
          -h, --help         display this help message
          -v, --version      display this program version
```

---

Run the `binary_tree` algorithm with long arguments:

```sh
mazebuildercli.exe --rows=25 --columns=25 --seed=42 --algorithm=binary_tree --output=bt.obj
```

Run the `dfs` algorithm with short arguments:

```sh
mazebuildercli.exe -r 25 -c 25 -s 42 -a dfs -o dfs.obj
```

Use the maze API in C++ programs:

```cpp
    #include <MazeBuilder/maze_builder.h>
    #include <iostream>

    void main(void) {

        mazes::maze_ptr m = mazes::factory::create();

        auto s = mazes::stringz::stringify(m);

        std::cout << s << std::endl;

        return 0;
    }
```

---

## CMake and configuration

[CMake 3.2](https://cmake.org) or greater is used as the build and test system.
The included examples require external dependencies:
  - SDL
  - SFML
  - box2d
  - Catch2
 
`CMake` can fetch these dependencies from their respective `git` repo's on the Internet.

Using the following CMake options to configure this project:


| CMake Option | Default | Description |
|--------------|---------|------------ |
| MAZE_BUILDER_EXAMPLES | OFF | Build with project examples enabled. |
| MAZE_BUILDER_COVERAGE | OFF | Build with code coverage using `CppCheck`. |
| MAZE_BUILDER_TESTS | OFF | Build with testing using `Catch2`. |
| MAZE_BUILDER_DOCS | OFF | Build the docs using `doxygen`. |
| MAZE_BUILDER_MEMCHECK | OFF | Build with `Valgrind` and `Memcheck` support. |
| CMAKE_TOOLCHAIN_FILE | `cmake` | Building with a specific toolchain. Useful for [`Emscripten`](#cmake-web-builds) builds. |

---

### CMake build

Configure the examples using a default `CMake` generator: `cmake -S . -B build-examples -DMAZE_BUILDER_EXAMPLES:BOOL=ON`

Build it: `cmake --build build-examples --config Release`

By default, both a shared-object library and static library are produced.
The shared and static files have different naming conventions depending on the platform:

| Platform | static lib | shared lib |
| -------- | ---- | ---- |
| Windows | `mazebuildercore_static.lib` | `mazebuildercore_shared.dll` |
| Linux | `libmazebuildercore_static.a` | `libmazebuildercore_shared.so` |
| MacOS | | |

### CTest tests builds

Configure the project for testing: `cmake -S . -B build-tests -DMAZE_BUILDER_TESTS:BOOL=ON`

Run the tests with a specific configuration: `ctest --test-dir build-tests/tests --verbose -C Debug`

### CMake Web builds

Configure the examples for the browser using [Emscripten](https://emscripten.org/) and their toolchain file.

```sh
cmake -S . -B build-web -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${my/emsdk/repo}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
```

---

## JSON payloads

The follow is an example of a JSON export: `mazebuildercli.exe -r 10 -c 10 -a binary_tree -o out.json`.

```json
{
  "rows": 10,
  "cols": 10,
  "depth": 1,
  "seed": 0,
  "str": "+---+---+---+---+---+\n
          |           |       |\n
          +   +---+   +---+   +\n
          |       |           |\n
          +---+---+---+---+---+\n",
  "distances": false
}
```

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

[Check out the this example in a web app!](https://jade-semifreddo-f24ef0.netlify.app/)

The web app can be run locally with the provided [secure_http_server.py](scripts/secure_http_server.py) script.
Once the provided script is running, then open the browser to `http://localhost:8000`.


## TODO | 6.0.1-build

 - [ ] JSON input and argument support | `JSONHelper` impl
 - [x] Update docs using `doxygen` formats
 - [ ] Joystick and Touch suppport
 - [x] Code coverage support with `cppcheck`
 - [ ] Add support and examples for SDL_GPU
 - [x] Simplify `grid_interface.h`
 - [x] Update version
 - [ ] Update tests to ensure new interface API works
 - [ ] Add "Snake" example
 - [ ] Update Voxels GUI
 - [x] Single include `#include <MazeBuilder/maze_builder.h>`

## More Learning Resources

 - [Maze Builder Docs](https://zmertens.github.io/mazebuilder.github.io/index.html)
 - [Mazes for Programmers Book](https://www.jamisbuck.org/mazes/)
 - [Craft](https://github.com/fogleman/Craft)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [SFML](https://github.com/SFML/SFML)
 - [Catch2](https://github.com/catchorg/Catch2)
