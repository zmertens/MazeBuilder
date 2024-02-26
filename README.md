# Maze Builder

Build mazes in console or 3D, and save, export to file.

## CMake

This project uses `cmake` to build and test. It uses `find_package` or `FetchContent` to get SDL, Catch2 and CURL libraries if necessary. Modern hardware supporting OpenGL 3.0 is required.

```sh
cmake -S "source directory here" -B build
cmake --build build/
```

| CMake Option | Default | 
|--------------|---------|
| BUILD_FOR_ANDROID | OFF |
| BUILD_SHARED_LIBS | TRUE |
| BUILD_STATIC_LIBS | FALSE |
| CMAKE_BUILD_TYPE | RelWithDebInfo |

## Commands

Generate a simple maze using `default` algorithms, and print the result to stdout:
```sh
./maze_builder > default_maze.txt
```

Specify a seed and a `binary search tree` algorithm:
```sh
./maze_builder --seed=1337 --algo=bst -o bst.txt
```

Make Maze Builder run as `interactive` with a `seed`:
```sh
./maze_builder -i -s 1337
```

## Scripts

The `scripts` directory contains Ruby and Python scripts to build mazes quickly.
These programs can output to `.png` files or `stdout`.

## Resources
 - [Mazes for Programmers Book](https://www.jamisbuck.org/mazes/)
 - [Craft](https://github.com/fogleman/Craft)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [Catch2](https://github.com/catchorg/Catch2)
