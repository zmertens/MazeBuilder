#include <MazeBuilder/distance_grid.h>

#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>

#include <queue>
#include <iostream>

using namespace mazes;
using namespace std;

/**
 * @brief Construct a new distance grid object with a generated maze
 * @param height 1
 */
distance_grid::distance_grid(unsigned int rows, unsigned int cols, unsigned int height)
	: grid::grid(rows, cols, height)
	, m_grid(make_shared<grid>(rows, cols, height))
	, m_distances(make_shared<distances>(this->m_binary_search_tree_root)) {
    this->calc_distances();
}


std::optional<std::string> distance_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
	if (m_distances) {
		const auto d = m_distances->operator[](c);
		if (d >= 0) {
			return to_base36(d);
		}
	}
	return this->m_grid->contents_of(c);
}

std::optional<std::uint32_t> distance_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
	return this->m_grid->background_color_for(cref(c));
}

optional<std::string> distance_grid::to_base36(int value) const {
	static constexpr auto base36_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::string result;
	do {
		result.push_back(base36_chars[value % 36]);
		value /= 36;
	} while (value > 0);
	std::reverse(result.begin(), result.end());
	return result;
}

/**
 * @brief Compute the distances between cells using 
 * 	Djikstra's shortest-path algorithm
 */
void distance_grid::calc_distances() noexcept {
	auto&& root = this->m_binary_search_tree_root;
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

/**
 * @brief Get the distances object
 * @return shared_ptr<distances>
 */
std::shared_ptr<distances> distance_grid::get_distances() const noexcept {
	return this->m_distances;
}
