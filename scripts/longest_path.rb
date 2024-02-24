# Find longest path, loop over maze twice

require 'distance_grid'
require 'binary_tree'

grid = DistanceGrid.new(50, 50)

BinaryTree.on(grid)

# Pick NW corner in grid
start = grid[0, 0]
distances = start.distances

new_start, distance = distances.max
new_distances = new_start.distances
goal, distance = new_distances.max

grid.distances = new_distances.path_to(goal)

puts grid