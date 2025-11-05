# MazeBuilder Project

## Overview
MazeBuilder scripts implements various algorithms for generating mazes.

## Files

### `scripts/mazes.rb`
This program implements varios maze algorithms from a book. It defines classes such as `Cell`, `Grid`, `Distances`, and several maze generation algorithms. It also includes methods for rendering the maze as a PNG image and calculating distances between cells.
Useful for making an app icon.

### `Gemfile`
This file is used for managing Ruby gem dependencies. It specifies the gems required for the project, such as `chunky_png` for image processing.

## Usage
To generate a maze, run the `mazes.rb` script. The data will be output as a PNG file in the current directory.

## Installation
To install the required gems, run:
```
bundle install
```
