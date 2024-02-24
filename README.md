# Maze Builder

Build mazes in 3D and save, export to file.

## Commands

Generate a simple maze using `default` algorithms, and print the result to stdout:
```sh
./maze_builder > default_maze.txt
```

Specify a seed and a `binary search tree` algorithm:
```sh
./maze_builder --seed=1337 --algo=bst > bst.txt
```

Make Maze Builder interactive with a seed:
```sh
./maze_builder -i -s 1337
```

## Scripts

The `scripts` directory contains Ruby and Python scripts to build mazes quickly.
Outputs to `.png` files or `stdout`.

## Resources
 - [Mazes for Programmers Book](https://www.jamisbuck.org/mazes/)
 - [Craft](https://github.com/fogleman/Craft)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [SDL](https://github.com/libsdl-org/SDL)
