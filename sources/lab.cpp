#include <MazeBuilder/lab.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/string_view_utils.h>

#include <random>
#include <sstream>
#include <vector>

using namespace mazes;

/// @brief 
lab::lab()
: m_p_q{}
, levels{} {

}

lab::~lab() = default;

lab::lab(const lab& other)
: m_p_q(other.m_p_q)
, levels{} {

}

lab& lab::operator=(const lab& other) {
    if (this == &other) {
        return *this;
    }
    m_p_q = other.m_p_q;
    return *this;
}

std::optional<std::tuple<int, int, int, int>> lab::find(int p, int q) const noexcept {
    auto itr = m_p_q.find({ p, q });
    return (itr != m_p_q.cend()) ? make_optional(itr->second) : std::nullopt;
}

void lab::insert(int p, int q, int r, int w) noexcept {
    m_p_q.insert_or_assign({ p, q }, std::make_tuple(p, q, r, w));
}

bool lab::empty() const noexcept {
    return m_p_q.empty();
}

int lab::get_levels() const noexcept {
    return levels;
}

void lab::set_levels(int levels) noexcept {
    this->levels = levels;
}

int lab::get_random_block_id() const noexcept {
    using namespace std;

    mt19937 mt{ 42681ul };
    auto get_int = [&mt](int low, int high) {
        uniform_int_distribution<int> dist{ low, high };
        return dist(mt);
        };

    return get_int(0, 23);
}


void lab::link(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi) noexcept {
    if (!c1 || !c2) return;

    // Add c2 to c1's links
    c1->add_link(c2);

    // If bidirectional, add c1 to c2's links
    if (bidi) {
        c2->add_link(c1);
    }
}

void lab::unlink(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi) noexcept {
    if (!c1 || !c2) return;

    // Remove c2 from c1's links
    c1->remove_link(c2);

    // If bidirectional, remove c1 from c2's links
    if (bidi) {
        c2->remove_link(c1);
    }
}

void lab::set_neighbors(configurator const& config, const std::vector<int>& indices, std::vector<std::shared_ptr<cell>>& cells_to_set) noexcept {

    using namespace std;

    const auto rows = config.rows();
    const auto columns = config.columns();
    const auto levels = config.levels();

    // Lambda function to calculate cell index
    auto calculate_cell_index = [rows, columns](unsigned int row, unsigned int col, unsigned int level) noexcept {
        return level * (rows * columns) + row * columns + col;
    };

    // Create cells
    const auto total_cells = rows * columns * levels;
    cells_to_set.reserve(total_cells);
    for (int index = 0; index < total_cells; ++index) {
        cells_to_set.emplace_back(std::make_shared<cell>(index));
    }

    // Reorder cells based on indices if provided
    if (!indices.empty()) {
        std::vector<shared_ptr<cell>> reordered_cells;
        reordered_cells.reserve(cells_to_set.size());
        for (int index : indices) {
            if (index >= 0 && index < static_cast<int>(cells_to_set.size())) {
                // Just copy the shared_ptr, don't create a new cell
                reordered_cells.push_back(cells_to_set[index]);
            }
        }
        if (reordered_cells.size() == cells_to_set.size()) {
            cells_to_set = std::move(reordered_cells);
        }
    }

    // Create topology
    std::unordered_map<int, std::unordered_map<Direction, int>> topology;
    for (unsigned int l = 0; l < levels; ++l) {
        for (unsigned int r = 0; r < rows; ++r) {
            for (unsigned int c = 0; c < columns; ++c) {
                int cell_index = calculate_cell_index(r, c, l);
                std::unordered_map<Direction, int> neighbors;

                // North neighbor
                if (r > 0) {
                    neighbors[Direction::NORTH] = calculate_cell_index(r - 1, c, l);
                }

                // South neighbor
                if (r < rows - 1) {
                    neighbors[Direction::SOUTH] = calculate_cell_index(r + 1, c, l);
                }

                // East neighbor
                if (c < columns - 1) {
                    neighbors[Direction::EAST] = calculate_cell_index(r, c + 1, l);
                }

                // West neighbor
                if (c > 0) {
                    neighbors[Direction::WEST] = calculate_cell_index(r, c - 1, l);
                }

                topology[cell_index] = neighbors;
            }
        }
    }

    // Set neighbor relationships
    for (auto& c : cells_to_set) {
        const auto& neighbors = topology[c->get_index()];
        for (const auto& [direction, neighbor_index] : neighbors) {
            if (neighbor_index >= 0 && neighbor_index < static_cast<int>(cells_to_set.size())) {
                c->add_link(cells_to_set[neighbor_index]);
            }
        }
    }
}
