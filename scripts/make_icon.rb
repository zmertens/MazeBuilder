# This script implements several maze algorithms from the book "Mazes for Programmers"

# Running the program outputs PNG files in the current directory
# Its also output to make an ASCII representation of the maze to the console

require 'chunky_png'

# Measure distances between grid cells
class Distances

    def initialize(root)
        @root = root
        @cells = {}
        @cells[@root] = 0
    end

    def [](cell)
        @cells[cell]
    end

    def []=(cell, distance)
        @cells[cell] = distance
    end

    def cells
        @cells.keys
    end

    def path_to(goal)
        current = goal
        breadcrumbs = Distances.new(@root)

        breadcrumbs[current] = @cells[current]

        until current == @root
            current.links.each do |neighbor|
                if @cells[neighbor] < @cells[current]
                    breadcrumbs[neighbor] = @cells[neighbor]
                    current = neighbor
                    break
                end
            end
        end
        breadcrumbs
    end

    def max
        max_distance = 0
        max_cell = @root
        @cells.each do |cell, distance|
            if distance > max_distance
                max_cell = cell
                max_distance = distance
            end
        end
        [max_cell, max_distance]
    end
end

class Cell
    attr_reader :row, :column
    attr_accessor :north, :south, :east, :west

    def initialize(row, column)
        @row, @column = row, column
        @links = {}
    end

    # Helper methods for linking and unlinking cells in mazes
    def link(cell, bidi=true)
        @links[cell] = true
        cell.link(self, false) if bidi
        self
    end

    def unlink(cell, bidi=true)
        @links.delete(cell)
        cell.unlink(self, false) if bidi
        self
    end

    def links
        @links.keys
    end

    def linked?(cell)
        @links.key?(cell)
    end

    def neighbors
        list = []
        list << north if north
        list << south if south
        list << east if east
        list << west if west
        list
    end

    def to_s
        "row: #{@row} , column: #{@column}"
    end

    # Compute distances between cell starting points
    # using Dijkstra's algo
    def distances
        distances = Distances.new(self)
        frontier = [self]
        while frontier.any?
            new_frontier = []
            frontier.each do |cell|
                cell.links.each do |linked|
                    next if distances[linked]
                    distances[linked] = distances[cell] + 1
                    new_frontier << linked
                end
            end
            frontier = new_frontier
        end
        distances
    end
end

class Grid
    attr_reader :rows, :columns

    def initialize(rows, columns)
        @rows = rows
        @columns = columns
        @grid = prepare_grid

        configure_cells

    end

    def prepare_grid
        Array.new(rows) do |row|
            Array.new(columns) do |column|
                Cell.new(row, column)
            end
        end
    end

    # Configure 2-dimensional grid starting with the top-leftmost cell,
    #   and count left-right and then downward (similar to an SQL table!)
    def configure_cells
        each_cell do |cell|
            row, col = cell.row, cell.column
            cell.north = self[row - 1, col]
            cell.south = self[row + 1, col]
            cell.west = self[row, col - 1]
            cell.east = self[row, col + 1]
        end
        puts "configured cells"
    end

    def [](row, column)
        return nil if row.nil? || column.nil?
        return nil unless row.between?(0, @rows - 1)
        return nil unless column.between?(0, @grid[row].count - 1)
        @grid[row][column]
    end

    def random_cell
        row = rand(@rows)
        column = rand(@grid[row].count)
        self[row, column]
    end

    def size
        @rows * @columns
    end
    
    def each_row
        @grid.each do |row|
            yield row
        end
    end

    def each_cell
        each_row do |row|
            row.each do |cell|
                yield cell if cell
            end
        end
    end

    def contents_of(cell)
        " "
    end

    def background_color_for(cell)
        nil
    end

    # ASCII-character display of maze
    def to_s
        output = "+" + "---+" * columns + "\n"
        each_row do |row|
            # Top and Bottom of Cell Wall
            top = "|"
            bottom = "+"
            row.each do |cell|
                cell = Cell.new(-1, -1) unless cell
                # body = "   "
                body = " #{contents_of(cell)} "
                east_boundary = (cell.linked?(cell.east) ? " " : "|")
                top << body << east_boundary
                south_boundary = (cell.linked?(cell.south) ? "   " : "---")
                corner = "+"
                bottom << south_boundary << corner
            end
            output << top << "\n"
            output << bottom << "\n"
        end
        output
    end

    def to_png(cell_size: 25)
        img_width = cell_size * columns
        img_height = cell_size * rows

        background = ChunkyPNG::Color::WHITE

        wall = ChunkyPNG::Color::BLACK

        img = ChunkyPNG::Image.new(img_width + 1, img_height + 1, background)

        [:backgrounds, :walls].each do |mode|
            each_cell do |cell|
                x1 = cell.column * cell_size
                y1 = cell.row * cell_size
                x2 = (cell.column + 1) * cell_size
                y2 = (cell.row + 1) * cell_size

                if mode == :backgrounds
                    color = background_color_for(cell)
                    img.rect(x1, y1, x2, y2, color, color) if color
                else
                    img.line(x1, y1, x2, y1, wall) unless cell.north
                    img.line(x1, y1, x1, y2, wall) unless cell.west
                
                    img.line(x2, y1, x2, y2, wall) unless cell.linked?(cell.east)
                    img.line(x1, y2, x2, y2, wall) unless cell.linked?(cell.south)
                end
            end
        end
        img
    end

    def deadends()
        deadends = []
        each_cell do |cell|
            deadends << cell if cell.links.count == 1
        end
        deadends
    end
end

class DistanceGrid < Grid
    attr_accessor :distances

    def contents_of(cell)
        if distances && distances[cell]
            distances[cell].to_s(36)
        else
            super
        end
    end
end

class ColoredGrid < Grid
    def distances=(distances)
        @distances = distances
        farthest, @maximum = distances.max
    end

    def background_color_for(cell)
        if !@distances[cell]
            return nil
        end
        distance = @distances[cell]
        intensity = (@maximum - distance).to_f / @maximum
        dark = (255 * intensity).round
        bright = 128 + (127 * intensity).round
        ChunkyPNG::Color::rgb(dark, bright, dark)
    end
end

class Mask
    attr_reader :rows, :columns

    def initialize(rows, columns)
        @rows, @columns = rows, columns
        @bits = Array.new(@rows){Array.new(@columns, true)}
    end

    def [](row, column)
        if row.between?(0, @rows - 1) && column.between?(0, @columns - 1)
            @bits[row][column]
        else
            false
        end
    end

    def []=(row, column, is_on)
        @bits[row][column] = is_on
    end

    def count
        count = 0
        @rows.times do |row|
            @columns.times do |col|
                count += 1 if @bits[row][col]
            end            
        end
        count
    end

    def random_location
        loop do
            row = rand(@rows)
            col = rand(@columns)
            return [row][col] if @bits[row][col]
        end
    end

    def self.from_png(file)
        image = ChunkyPNG::Image.from_file(file)
        mask = Mask.new(image.height, image.width)

        mask.rows.times do |row|
            mask.columns.times do |col|
                if image[col, row] == ChunkyPNG::Color::BLACK
                    mask[row, col] = false
                else
                    mask[row, col] = true
                end
            end
        end
        mask
    end
end

class MaskedGrid < Grid
    attr_reader :mask

    def initialize(mask)
        @mask = mask
        super(@mask.rows, @mask.columns)
    end

    def prepare_grid
        Array.new(rows) do |row|
            Array.new(columns) do |column|
                Cell.new(row, column) if @mask[row, column]
            end
        end
    end

    def random_cell
        row, col = @mask.random_location
        self[row, col]
    end

    def size
        @mask.count
    end

end

class BinaryTree
    def self.on(grid)
        grid.each_cell do |cell|
            neighbors = []
            neighbors << cell.north if cell.north
            neighbors << cell.east if cell.east
            index = rand(neighbors.length).floor
            neighbor = neighbors[index]
            cell.link(neighbor) if neighbor
        end
    end
end

class Sidewinder

    def self.on(grid)
        grid.each_row do |row|
            run = []
            row.each do |cell|
                run << cell
                at_eastern_boundary = (cell && cell.east == nil) ? true : false
                at_northern_boundary = cell && cell.north == nil ? true : false
                should_close_out = at_eastern_boundary || (!at_northern_boundary && rand(2) == 0)

                if should_close_out
                    member = run.sample
                    member.link(member.north) if member && member.north
                    run.clear
                else
                    cell.link(cell.east) if cell != nil && cell.east
                end
            end
        end
        grid
    end

end

class AldousBroder

    def self.on(grid)
        cell = grid.random_cell
        unvisited = grid.size - 1
        while unvisited > 0
            neighbor = cell.neighbors.sample
            if neighbor.links.empty?
                cell.link(neighbor)
                unvisited -= 1
            end
            cell = neighbor
        end
        grid
    end

end

class Wilsons

    def self.on(grid)
        unvisited = []
        grid.each_cell { |cell| unvisited << cell }
        first = unvisited.sample
        unvisited.delete(first)
        while unvisited.any?
            cell = unvisited.sample
            path = [cell]
            while unvisited.include?(cell)
                cell = cell.neighbors.sample
                position = path.index(cell)
                if position
                    path = path[0..position]
                else
                    path << cell
                end
            end
            0.upto(path.length - 2) do |index|
                path[index].link(path[index + 1])
                unvisited.delete(path[index])
            end
        end
        grid
    end
end

class HuntAndKill
    def self.on(grid)
        current = grid.random_cell
        while current
            unvisited_neighbors = current.neighbors.select { |n| n.links.empty? }
            # Kill phase
            if unvisited_neighbors.any?
                neighbor = unvisited_neighbors.sample
                current.link(neighbor)
                current = neighbor
            else
                current = nil
                # Hunt phase
                grid.each_cell do |cell|
                    visited_neighbors = cell.neighbors.select{ |n| !n.links.empty? }
                    if cell.links.empty? && visited_neighbors.any?
                        current = cell
                        neighbor = visited_neighbors.sample
                        current.link(neighbor)
                        break
                    end
                end
            end
        end
        grid
    end
end

class RecursiveBacktracker
    def self.on(grid, start_at: grid.random_cell)
        s = []
        s.push(start_at)
        while s.any?
            current = s.last
            neighbors = current.neighbors.select { |n| n.links.empty? }
            if neighbors.empty?
                s.pop
            else
                neighbor = neighbors.sample
                current.link(neighbor)
                s.push(neighbor)
            end
        end
        grid
    end
end

1.times do |n|

    grid = ColoredGrid.new(50, 35)
    RecursiveBacktracker.on(grid)

    end1 = grid[grid.rows / 2, grid.columns / 2]
    start1 = grid[grid.rows - 1, grid.columns - 1]

    grid.distances = end1.distances.path_to(start1)

    filename = "recbak%02d.png" % n
    grid.to_png.save(filename)

    #filename = "mask.png"

    #mask = Mask.from_png(filename)
    #grid = MaskedGrid.new(mask)

    #Sidewinder.on(grid)

    #grid.to_png(cell_size: 5).save("output2.png")

    puts grid
    puts "writing to: #{filename}"
    puts "Deadends: #{grid.deadends.count}"
end
