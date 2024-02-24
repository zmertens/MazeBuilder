# Apply Aldous-Broder maze building algo and print to colored image

require 'colored_grid'
require 'AldousBroder'

6.times do |n|
    grid = ColoredGrid.new(20, 20)

    AldousBroder.on(grid)
    
    # Pick a starting cell in middle of maze
    middle = grid[grid.rows / 2, grid.columns / 2]

    grid.distances = middle.distances

    filename = "aldous_broder_%02d.png" % n
    
    grid.to_png.save(filename)
    
    puts "saved to #{filename}"
    
end
