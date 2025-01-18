![Release screenshot](examples/Voxels/textures/maze_in_green_32x32.bmp)

# Maze Builder

Build different representations of mazes quickly and easily on multiple platforms and languages.

Below is an example string output from the [command-line interface example](#examples-and-scripts).
```
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

The library provides multiple export formats like Wavefront object, PNG image, and plain text or `stdout`.
Emphasis on rapid prototyping the design process by going from one example maze to the next.
These exports can then be integrated into game engines and renderers like `Unity`, `Godot`, `Blender`, `Unreal Engine` and so forth.

## Commands and Help Message

```sh
        Usages: mazebuildercli.exe [OPTION(S)]... [OUTPUT]
          Program generates string representations of mazes with export formats
        Example: mazebuildercli.exe -r 10 -c 10 -a binary_tree > out_maze.txt
          -a, --algorithm    dfs, sidewinder, binary_tree [default]
          -s, --seed         seed for the mt19937 generator
          -r, --rows         maze rows
          --height           maze height
          -c, --columns      maze columns
          -d, --distances    show distances between cells as integers
          -o, --output       [.txt], [.png], [.obj], [stdout[default]]
          -h, --help         display this help message
          -v, --version      display program version
```

### Some example run commans (requires the [`cli` example](#examples-and-scripts))

Run the `binary_tree` maze-generating algorithm with long arguments:
```sh
mazebuildercli.exe --rows=25 --columns=25 --seed=42 --algorithm=binary_tree --output=bt.obj
```

Run the `dfs` maze-generating algorithm with short arguments:
```sh
mazebuildercli.exe -r 25 -c 25 -s 42 -a binary_tree -o dfs.obj
```

## CMake and building the libraries

This project uses [CMake 3.2](https://cmake.org) or greater as the build and test system.
Only the [examples](#examples-and-scripts) require external dependencies not contained within the repo.
`CMake` can fetch the dependencies from their respective `git` repo's on the Internet.

The first step is to clone this repo, and then change directories to the root of it.

Using the following CMake options to configure this project:


| CMake Option | Default | Description |
|--------------|---------|------------ |
| MAZE_BUILDER_EXAMPLES | OFF | Build with examples enabled. |
| MAZE_BUILDER_COVERAGE | OFF | Build with code coverage using `CppCheck`. |
| MAZE_BUILDER_TESTS | OFF | Build with testing using `Catch2`. |
| MAZE_BUILDER_MEMCHECK | OFF | Build `Valgrind` and `Memcheck` support. |
| CMAKE_TOOLCHAIN_FILE | `cmake` | Building with a specific toolchain. Useful for `Emscripten` builds. |

---

Configure the examples using default generator: `cmake -S . -B build-examples -DMAZE_BUILDER_EXAMPLES:BOOL=ON`

Build it: `cmake --build build-examples --config Release`

By default, both a shared-object library and static library are produced.
The shared and static files have different naming conventions depending on the platform:

| Platform | static lib | shared lib |
| -------- | ---- | ---- |
| Windows | `mazebuildercore_static.lib` | `mazebuildercore_shared.dll` |
| Linux | `libmazebuildercore_static.a` | `libmazebuildercore_shared.so` |
| MacOS | | |

### Test:
`ctest --test-dir build --verbose`

### Web:

Configure the examples for the browser using [Emscripten](https://emscripten.org/) and their toolchain file:

```sh
cmake -S . -B . -DCMAKE_TOOLCHAIN_FILE=${my/emsdk/repo}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake 
```

Where `${my/emsdk/repo}` is the directory containing the repo for `Emscripten`.

## Examples and scripts

### CLI
The command line program with aforementioned [commands](#commands-and-help-message).

### Http
Send HTTP requests over a network with a payload in JSON format, containing mazes.

### mazebuilderjs

### Scripts

The Ruby script `ruby mazes.rb` generates PNG images of mazes using algorithms similar to the C++ library.
It is a good place to start learning about the maze-generating algorithms.
The Python script `python3 solver.py` plays with the maze generation by loading PNG files and finding paths and networks.

**Install dependencies:**
  - `gem install chunky_png`
  - `pip install numpy pillow networkx`

### Snakegen

### Voxels

[Check out the Voxel example as a web app!](https://jade-semifreddo-f24ef0.netlify.app/)

The web app can be run locally with the provided [secure_http_server.py](scripts/secure_http_server.py) script, and then open the browser to `http://localhost:8000`.


## TODO | 6.0.1-build

 - [ ] JSON input and argument support | `JSONHelper` impl
 - [ ] Update docs using `doxygen` formats
 - [ ] Joystick and Touch suppport
 - [ ] Code coverage support with `cppcheck`
 - [ ] Add support and examples for SDL_GPU
 - [ ] Simplify `grid_interface.h`
 - [ ] Update version
 - [ ] Better image writing and proper classes like `line`
 - [ ] Update tests to ensure new interface API works
 - [ ] Add support to export to bool 2D array (+ inversion)
 - [ ] Add "Snake" example
 - [ ] Single include `#include <MazeBuilder/maze_builder.h>`

## Resources
 - [Mazes for Programmers Book](https://www.jamisbuck.org/mazes/)
 - [Craft](https://github.com/fogleman/Craft)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [Catch2](https://github.com/catchorg/Catch2)
