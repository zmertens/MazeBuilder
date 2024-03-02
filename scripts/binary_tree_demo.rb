# Runs the BinaryTree algorithm and displays

# gem install chunky_png
# ruby -I . binary_tree_demo.rb

require 'grid'
require 'binary_tree'

grid = Grid.new(5, 5)

BinaryTree.on(grid)

puts grid

img = grid.to_png
img.save "binary_tree_demo_maze.png"