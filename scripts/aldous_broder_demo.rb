# Apply Aldous-Broder maze building algo and print to image

require 'grid'
require 'AldousBroder'

grid = Grid.new(20, 20)

AldousBroder.on(grid)

filename = "aldous_broder.png"

grid.to_png.save(filename)

puts "saved to #{filename}"
