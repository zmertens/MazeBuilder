# Maze Builder

Build and download mazes interactively or using the command-line interface (CLI).
This project emphasizes conveniance and simplicity in order to achieve a tangible maze in Wavefront object or ASCII formats which can then be integrated into game engines, renderers, or data-processing systems.

[Check out the web app!](https://jade-semifreddo-f24ef0.netlify.app/)

![Release screenshot](textures/maze_builder_release1.png)

## CMake

This project uses `cmake` to build and test. It uses `find_package` or `FetchContent` to get SDL and Catch2 if necessary. Requires modern hardware supporting pthreads and OpenGL 3.0. The SDL library is used for portability and providing a window to draw to. Catch2 is used for test and supports modern C++11 conventions and above.

```sh
cmake -S ${my/mazebuilder/repo} -B build
cmake --build build/
```

where `${my/mazebuilder/repo}` is the directory containg the Git repo for Maze Builder.

| CMake Option | Default | Description |
|--------------|---------|-------------
| BUILD_TESTS | OFF | Build with maze algorithm testing via Catch2. |
| CMAKE_TOOLCHAIN_FILE | `cmake` | Building with a specific toolchain. Useful for Emscripten builds. |
| CMAKE_BUILD_TYPE | RelWithDebInfo | The build type can determine compiler optimizations and switches. |


Additionally, the Maze Builder can be built for the web using [Emscripten](https://emscripten.org/). Build with toolchain file:

```sh
cmake -S . -B . -DCMAKE_TOOLCHAIN_FILE=${my/emsdk/repo}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake 
```

Where `${my/emsdk/repo}` is the directory containing the Git repo for Emscripten's emsdk.
Run a local server using the included [secure_http_server.py](secure_http_server.py) file and modifying the [minimal shell](deps/emscripten_local/shell_minimal.html) to not run asynchrously.

## Commands and Help Message

```sh
Usages: maze_builder [OPTION]... [OUT_FILE]
Run to generate a maze based on a specific algorithm (using -a to specify)
Example: maze_builder -w 10 -l 10 -a binary_tree > my_bt_maze.txt
Options specify how to generate the maze and file output:
  -a, --algorithm    binary_tree [default], sidewinder
  -s, --seed         seed for the random number generator [mt19937]
  -w, --width        maze width [default=100]
  -y, --height       maze height [default=10]
  -l, --length       maze length [default=100]
  -i, --interactive  run program in interactive mode with a GUI
  -o, --output       stdout [default], plain text [.txt] or Wavefront object format [.obj]
  -h, --help         display this help message
  -v, --version      display program version
```

Specify a seed and a `binary tree` algorithm:
```sh
./maze_builder --seed=1337 --algorithm=binary_tree -o bt.txt
```

Make Maze Builder run as `interactive` with a `seed`:
```sh
./maze_builder -i -s 1337
```

## Scripts

The `scripts` directory contains Ruby and Python scripts to build mazes quickly.
These programs can output to `.png` files or `stdout`. There is a Python script
using the Blender API to generate a maze in that interface.

## Resources and Dependencies
 - [Mazes for Programmers Book](https://www.jamisbuck.org/mazes/)
 - [Craft](https://github.com/fogleman/Craft)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [Catch2](https://github.com/catchorg/Catch2)
 - [Emscripten](https://emscripten.org/)
