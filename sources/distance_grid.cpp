#include <MazeBuilder/distance_grid.h>

#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>

#include <queue>
#include <random>

using namespace mazes;

/// @brief Constructs a distance_grid object with specified dimensions and initializes the distance calculations.
/// @param rows 1
/// @param cols 1
/// @param levels 1
distance_grid::distance_grid(unsigned int rows, unsigned int cols, unsigned int height)
	: grid::grid(rows, cols, height) {

}

/// @brief Constructs a distance_grid object with specified dimensions and initializes the distance calculations.
/// @return future to init task
std::future<bool> distance_grid::get_future() noexcept {
    using namespace std;

    mt19937 rng{ 42681ul };
    static auto get_int = [&rng](int low, int high) ->int {
        uniform_int_distribution<int> dist{ low, high };
        return dist(rng);
        };

    auto [ROWS, COLUMNS, _] = this->get_dimensions();

    vector<int> shuffled_indices;
    shuffled_indices.resize(ROWS * COLUMNS);
    fill(shuffled_indices.begin(), shuffled_indices.end(), 0);
    unsigned int next_index{ 0 };
    for (auto itr{ shuffled_indices.begin() }; itr != shuffled_indices.end(); itr++) {
        *itr = next_index++;
    }

    return std::async(std::launch::async, [this, shuffled_indices]() mutable {
        this->configure_nodes(cref(shuffled_indices));

        m_distances = make_shared<distances>(this->m_binary_search_tree_root->cell_ptr);

        this->calc_distances();

        return true;
        });
}

std::optional<std::string> distance_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
	if (m_distances) {
		const auto d = m_distances->operator[](c);
		if (d >= 0) {
			return to_base36(d);
		}
	}
	return grid::contents_of(c);
}

std::optional<std::uint32_t> distance_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
	return grid::background_color_for(cref(c));
}

std::optional<std::string> distance_grid::to_base36(int value) const {
	static constexpr auto base36_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::string result;
	do {
		result.push_back(base36_chars[value % 36]);
		value /= 36;
	} while (value > 0);
	std::reverse(result.begin(), result.end());
	return result;
}

/// @brief Compute the distances between cells given a root cell
/// @details Uses Djikstra's shortest-path algorithms
void distance_grid::calc_distances() noexcept {
    using namespace std;

    if (!this->m_binary_search_tree_root) {
        return;
    }

	auto&& root = this->m_binary_search_tree_root->cell_ptr;
	queue<shared_ptr<cell>> frontier;
	frontier.push(root);
	m_distances->set(root, 0);
	// apply shortest path algorithm
	while (!frontier.empty()) {

		auto current = frontier.front();
		frontier.pop();
		auto current_distance = m_distances->operator[](current);
		for (const auto& neighbor : current->get_neighbors()) {

			if (!m_distances->contains(neighbor)) {
				m_distances->set(neighbor, m_distances->operator[](current) + 1);
				frontier.push(neighbor);
			}
		}
	}
}

std::shared_ptr<distances> distance_grid::get_distances() const noexcept {
	return this->m_distances;
}
