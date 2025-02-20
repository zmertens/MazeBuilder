# MazeBuilder Project

## Overview
MazeBuilder implements various algorithms for generating mazes. It allows users to create mazes using different techniques and visualize them as PNG images. Additionally, the project supports the functionality of using an image mask to define the structure of the maze.

## Files

### `scripts/mazes.rb`
This file contains the main logic for generating mazes using various algorithms. It defines classes such as `Cell`, `Grid`, `Distances`, and several maze generation algorithms. It also includes methods for rendering the maze as a PNG image and calculating distances between cells.

### `scripts/mask.png`
This is a PNG image file that will be used as a mask for parsing to create a maze. The colors in the image will determine the walls and paths of the maze.

### `Gemfile`
This file is used for managing Ruby gem dependencies. It specifies the gems required for the project, such as `chunky_png` for image processing.

## Usage
To generate a maze, run the `mazes.rb` script. You can specify the mask image file to create a maze based on the image's pixel data. The maze will be output as a PNG file in the current directory.

## Features
- Multiple maze generation algorithms (e.g., Recursive Backtracker, Aldous-Broder, etc.)
- Ability to generate mazes from image masks
- Output of mazes as PNG images
- Distance calculations between cells

## Future Enhancements
- Support for additional image formats (e.g., JPEG)
- More maze generation algorithms
- User interface for easier interaction with the maze generation process

## Installation
To install the required gems, run:
```
bundle install
```

## Contributing
Contributions are welcome! Please feel free to submit a pull request or open an issue for any enhancements or bug fixes.

