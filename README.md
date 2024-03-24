# Maze Builder

Build mazes in console or 3D, and save, export to file.

![Textured maze with solution in white](textures/maze_in_green_32x32.bmp)

## CMake

This project uses `cmake` to build and test. It uses `find_package` or `FetchContent` to get SDL and Catch2 if necessary. Modern hardware supporting OpenGL 3.0 is required.

```sh
cmake -S "source directory here" -B build
cmake --build build/
```

| CMake Option | Default | 
|--------------|---------|
| BUILD_MAZE_TESTS | OFF |
| BUILD_SHARED_LIBS | TRUE |
| BUILD_STATIC_LIBS | FALSE |
| CMAKE_BUILD_TYPE | RelWithDebInfo |

## Commands and Help Message

```sh
Usages: maze_builder [OPTION]... [OUT_FILE]
Run the builder to generate mazes from optional algorithms
Example: maze_builder -w 10 -l 10 -a binary_tree > out_maze.txt
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

## Resources
 - [Mazes for Programmers Book](https://www.jamisbuck.org/mazes/)
 - [Craft](https://github.com/fogleman/Craft)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [Catch2](https://github.com/catchorg/Catch2)
