# Maze Builder

Build and download mazes interactively or using the command-line interface (CLI).
Export the maze straight to a Wavefront object or PNG file.
Rapidly prototype the design process by going from example level to polished scene.
Exports can then be integrated into game engines and renderers like Unity, Godot, Blender and so forth.

![Release screenshot](scripts/wilsons_maze.png)

## Commands and Help Message

```sh
        Usages: maze_builder.exe [OPTION(S)]... [OUTPUT]
        Generates mazes and exports to different formats
        Example: maze_builder.exe -w 10 -l 10 -a binary_tree > out_maze.txt
          -a, --algorithm    dfs, sidewinder, binary_tree [default]
          -s, --seed         seed for the mt19937 generator [default=0]
          -w, --width        maze width [default=100]
          -y, --height       maze height [default=10]
          -l, --length       maze length [default=100]
          -c, --cell_size    maze cell size [default=3]
          -i, --interactive  run program in interactive mode with a GUI
          -o, --output       [.txt], [.png], [.obj], [stdout[default]]
          -h, --help         display this help message
          -v, --version      display program version
```

Specify a seed with the `binary_tree` maze-generating algorithm:
```sh
maze_builder.exe --seed=1337 --algorithm=binary_tree -o bt.obj
```

Run Maze Builder in an `interactive` mode using the [SDL](https://github.com/libsdl-org/SDL) app:
```sh
maze_builder.exe -i
```

## CMake

This project uses [CMake](https://cmake.org) 3.2 or greater as the build and test system. CMake can use `FetchContent` to get [SDL](https://github.com/libsdl-org/SDL) and [Catch2](https://github.com/catchorg/Catch2) from the web.
The SDL app requires modern hardware supporting OpenGL 3.0 or greater.

The first build step is to navigate to this local repo where it was cloned.

The following are the CMake options I use for this project:


| CMake Option | Default | Description |
|--------------|---------|-------------
| BUILD_MAZE_TESTS | OFF | Build with testing `maze_builder_lib` via Catch2. |
| CMAKE_CXX_COMPILER | `cmake` | Building with a specific compiler: `clang++`, `g++`, or `em++`. |
| CMAKE_TOOLCHAIN_FILE | `cmake` | Building with a specific toolchain. Useful for Emscripten builds. |
| CMAKE_BUILD_TYPE | RelWithDebInfo | The build type is case-sensitive. It can determine compiler optimizations and performance. `MinSizeRel, Release, RelWithDebInfo, Debug`. |

Configure the CMake files:

```sh
cmake -S . -B build -DCMAKE_CXX_COMPILER:STRING=clang++ -DCMAKE_BUILD_TYPE:STRING=Release -DBUILD_MAZE_TESTS:BOOLEAN=1
```

Build:
`cmake --build build`

Test:
`ctest --test-dir build --verbose`


Configure for the web using [Emscripten](https://emscripten.org/) and their toolchain file:

```sh
cmake -S . -B . -DCMAKE_TOOLCHAIN_FILE=${my/emsdk/repo}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake 
```

Where `${my/emsdk/repo}` is the directory containing the Git repo for Emscripten.

## Scripts

There is a Ruby script `ruby mazes.rb` and a Python script `python3 solver.py` to play with
maze generation and finding paths. The Ruby script is expected to generate PNG files representing mazes, and then the Python script loads PNG files and tries to determine if a maze exists.

**Dependenceis**
  - `gem install chunky_png`
  - `pip install numpy pillow networkx`

## Web application

[Check out the web app!](https://jade-semifreddo-f24ef0.netlify.app/)

The web app can be run locally with the provided [secure_http_server.py](secure_http_server.py) script, and then open the browser to `http://localhost:8000`.

## Resources
 - [Mazes for Programmers Book](https://www.jamisbuck.org/mazes/)
 - [Full Stack Dev Book](https://www.packtpub.com/en-us/product/full-stack-development-with-spring-boot-and-react-9781801816786)
 - [Craft](https://github.com/fogleman/Craft)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [Catch2](https://github.com/catchorg/Catch2)
 - [Emscripten](https://emscripten.org/)
 - [stb](https://github.com/nothings/stb)
  