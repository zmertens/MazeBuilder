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

Run the `binary_tree` algorithm with long arguments:
```sh
mazebuildercli.exe --rows=25 --columns=25 --seed=42 --algorithm=binary_tree --output=bt.obj
```

Run the `dfs` algorithm with short arguments:
```sh
mazebuildercli.exe -r 25 -c 25 -s 42 -a dfs -o dfs.obj
```

## CMake and building the libraries

This project uses [CMake 3.2](https://cmake.org) or greater as the build and test system.
Only the [examples](#examples-and-scripts) require external dependencies not contained within the repo.
`CMake` can fetch the dependencies from their respective `git` repo's on the Internet.

The first step is to clone this repo, and then change directories to the root of it.

Using the following CMake options to configure this project:


| CMake Option | Default | Description |
|--------------|---------|------------ |
| MAZE_BUILDER_EXAMPLES | OFF | Build with project examples enabled. |
| MAZE_BUILDER_COVERAGE | OFF | Build with code coverage using `CppCheck`. |
| MAZE_BUILDER_TESTS | OFF | Build with testing using `Catch2`. |
| MAZE_BUILDER_DOCS | OFF | Build the docs using `doxygen`. |
| MAZE_BUILDER_MEMCHECK | OFF | Build with `Valgrind` and `Memcheck` support. |
| CMAKE_TOOLCHAIN_FILE | `cmake` | Building with a specific toolchain. Useful for `Emscripten` builds. |

---

### Configure and build

Configure the examples using a default `CMake` generator: `cmake -S . -B build-examples -DMAZE_BUILDER_EXAMPLES:BOOL=ON`

Build it: `cmake --build build-examples --config Release`

By default, both a shared-object library and static library are produced.
The shared and static files have different naming conventions depending on the platform:

| Platform | static lib | shared lib |
| -------- | ---- | ---- |
| Windows | `mazebuildercore_static.lib` | `mazebuildercore_shared.dll` |
| Linux | `libmazebuildercore_static.a` | `libmazebuildercore_shared.so` |
| MacOS | | |

### Tests

Configure the project for testing: `cmake -S . -B build-tests -DMAZE_BUILDER_TESTS:BOOL=ON`

Run the tests with a specific configuration: `ctest --test-dir build-tests/tests --verbose -C Debug`

### Web

Configure the examples for the browser using [Emscripten](https://emscripten.org/) and their toolchain file.
```sh
cmake -S . -B build-web -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${my/emsdk/repo}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
```


Where `${my/emsdk/repo}` is the directory containing the repo for `Emscripten`.

## Examples and scripts

### As a C++ library

```cpp
#include <MazeBuilder/maze_builder.h>
#include <iostream>

int main(void) {
  mazes::maze_builder builder;
  mazes::maze_ptr my_maze = 
    builder.seed(1).algo(mazes::algos::DFS)
    .rows(10).columns(10).build();
  
  std::cout << *maze_ptr.get() << std::endl;

  return 0;
}
```


### CLI
The command line program with the aforementioned [commands](#commands-and-help-message).

### Http
Send HTTP requests over a network with a payload in JSON format, containing mazes.

```json
{
  "desc": ""
  "str": ""
}
```

### mazebuilderjs

```js
import Module from "./maze_builder_js.js";

class Maze {
	constructor(rows, cols, depth, seed, algorithm, output) {
		this.rows = rows;
		this.cols = cols;
		this.depth = depth;
		this.seed = seed;
		this.algorithm = algorithm;
		this.output = output;
	}
}

let libptr = null;

const run = async () => {
	const activeModule = await Module();
	if (activeModule) {
		libptr = activeModule.maze.get_instance(10, 10, 1, 0, "binary_tree", "Maze");
		if (libptr) {
			const my_maze = new Maze(libptr.rows, libptr.cols, libptr.depth,
				libptr.seed, libptr.algorithm, libptr.output);
			console.log(my_maze);
			const s = JSON.stringify(my_maze);
			console.log(s);
			libptr.delete();
		} else {
			console.error("No lib ptr");
		}
	} else {
		console.error("Failed to create Lib instance");
	}
}

run();
```

### Scripts

The Ruby script `mazes.rb` generates PNG images of mazes using algorithms similar to the C++ library.
It is a good place to start learning about the maze-generating algorithms.
The Python script `solver.py` plays with the maze generation by loading PNG files and finding paths and networks.

**Install dependencies:**
  - `gem install chunky_png`
  - `pip install numpy pillow networkx`

### Snake

A game of snake that uses the maze as a backbone for physics simulations.

### Voxels

![](https://media1.giphy.com/media/v1.Y2lkPTc5MGI3NjExbjlnbjl6NmZ3c3hmMW05MDV1YXg1NjFuOW5ydHRlYW5xdjVvY3BsMCZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/iO02l5jhramJ43olgE/giphy.gif)

A voxel world that lets you build mazes and exports.

[Check out the Voxels example in a web app!](https://jade-semifreddo-f24ef0.netlify.app/)

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

## Docs and Resources

 - [Maze Builder Docs](https://zmertens.github.io/mazebuilder.github.io/index.html)
 - [Mazes for Programmers Book](https://www.jamisbuck.org/mazes/)
 - [Craft](https://github.com/fogleman/Craft)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [SFML](https://github.com/SFML/SFML)
 - [Catch2](https://github.com/catchorg/Catch2)
