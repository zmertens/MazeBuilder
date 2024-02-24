# Apply Wilson's algo to maze building (loop-erasing technique)

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