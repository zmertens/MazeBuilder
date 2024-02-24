# Apply Aldous-Broder maze building algo

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