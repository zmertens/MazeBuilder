#include <MazeBuilder/cell_factory.h>

#include <algorithm>
#include <random>

using namespace mazes;

std::vector<std::shared_ptr<cell>> cell_factory::create_cells(
    const std::tuple<unsigned int, unsigned int, unsigned int>& dimensions) const noexcept {
    
    auto [rows, columns, levels] = dimensions;
    return create_cells(rows, columns, levels);
}

std::vector<std::shared_ptr<cell>> cell_factory::create_cells(
    unsigned int rows, unsigned int columns, unsigned int levels) const noexcept {
    
    std::vector<std::shared_ptr<cell>> cells;
    const auto total_cells = rows * columns * levels;
    cells.reserve(total_cells);
    
    for (int index = 0; index < total_cells; ++index) {
        cells.push_back(std::make_shared<cell>(index));
    }
    
    return cells;
}

void cell_factory::configure(
    std::vector<std::shared_ptr<cell>>& cells,
    const std::tuple<unsigned int, unsigned int, unsigned int>& dimensions,
    const std::vector<int>& indices) const noexcept {
    
    // If we have indices, reorder cells based on them
    if (!indices.empty()) {
        std::vector<std::shared_ptr<cell>> reordered_cells;
        reordered_cells.reserve(cells.size());
        
        for (int index : indices) {
            if (index >= 0 && index < static_cast<int>(cells.size())) {
                reordered_cells.push_back(cells[index]);
            }
        }
        
        // If we have a valid reordering, use it
        if (reordered_cells.size() == cells.size()) {
            cells = std::move(reordered_cells);
        }
    }
    
    // Create cell map and topology
    auto cell_map = create_cell_map(cells);
    auto topology = create_topology(cells, dimensions);
    
    // Set neighbor relationships
    set_cell_neighbors(cells, cell_map, topology);
}

std::unordered_map<int, std::shared_ptr<cell>> cell_factory::create_cell_map(
    const std::vector<std::shared_ptr<cell>>& cells) const noexcept {
    
    std::unordered_map<int, std::shared_ptr<cell>> cell_map;
    cell_map.reserve(cells.size());
    
    for (const auto& cell_ptr : cells) {
        if (cell_ptr) {
            cell_map[cell_ptr->get_index()] = cell_ptr;
        }
    }
    
    return cell_map;
}

std::unordered_map<int, std::unordered_map<Direction, int>> cell_factory::create_topology(
    const std::vector<std::shared_ptr<cell>>& cells,
    const std::tuple<unsigned int, unsigned int, unsigned int>& dimensions) const noexcept {
    
    auto [rows, columns, levels] = dimensions;
    std::unordered_map<int, std::unordered_map<Direction, int>> topology;
    
    for (unsigned int l = 0; l < levels; ++l) {
        for (unsigned int r = 0; r < rows; ++r) {
            for (unsigned int c = 0; c < columns; ++c) {
                int cell_index = calculate_cell_index(r, c, l, dimensions);
                std::unordered_map<Direction, int> neighbors;
                
                // North neighbor
                if (r > 0) {
                    neighbors[Direction::NORTH] = calculate_cell_index(r - 1, c, l, dimensions);
                }
                
                // South neighbor
                if (r < rows - 1) {
                    neighbors[Direction::SOUTH] = calculate_cell_index(r + 1, c, l, dimensions);
                }
                
                // East neighbor
                if (c < columns - 1) {
                    neighbors[Direction::EAST] = calculate_cell_index(r, c + 1, l, dimensions);
                }
                
                // West neighbor
                if (c > 0) {
                    neighbors[Direction::WEST] = calculate_cell_index(r, c - 1, l, dimensions);
                }
                
                // Add more directions for 3D mazes as needed
                
                topology[cell_index] = neighbors;
            }
        }
    }
    
    return topology;
}

int cell_factory::calculate_cell_index(
    unsigned int row, unsigned int col, unsigned int level,
    const std::tuple<unsigned int, unsigned int, unsigned int>& dimensions) const noexcept {
    
    auto [rows, columns, levels] = dimensions;
    return level * (rows * columns) + row * columns + col;
}

void cell_factory::set_cell_neighbors(
    std::vector<std::shared_ptr<cell>>& cells,
    const std::unordered_map<int, std::shared_ptr<cell>>& cell_map,
    const std::unordered_map<int, std::unordered_map<Direction, int>>& topology) const noexcept {
    
    // Store the topology in the factory for later use by the grid
    // The grid will need this topology information when set_cells is called
    m_topology = topology;
}

const std::unordered_map<int, std::unordered_map<Direction, int>>& cell_factory::get_topology() const noexcept {
    return m_topology;
}
