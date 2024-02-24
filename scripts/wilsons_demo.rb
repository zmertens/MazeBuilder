# Apply Wilsons maze-building algo (no color)

require 'grid'
require 'Wilsons'

grid = Grid.new(20, 20)
Wilsons.on(grid)

filename = "wilsons.png"

grid.to_png.save(filename)

puts "Saving Wilsons demo to #{filename}"