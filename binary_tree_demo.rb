# Runs the BinaryTree algorithm and displays

require 'grid'
require 'binary_tree'

grid = Grid.new(4, 4)

BinaryTree.on(grid)

puts grid

# img = grid.to_png
# img.save "binary_tree_demo_maze.png"