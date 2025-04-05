#include <MazeBuilder/distance_grid.h>

#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>

#include <queue>
#include <random>
#include <algorithm>
#include <numeric>

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
    static auto get_int = [&rng](int low, int high) -> int {
        uniform_int_distribution<int> dist{ low, high };
        return dist(rng);
        };

    auto [ROWS, COLUMNS, _] = this->get_dimensions();

    vector<int> shuffled_indices(ROWS * COLUMNS);
    iota(shuffled_indices.begin(), shuffled_indices.end(), 0);
    shuffle(shuffled_indices.begin(), shuffled_indices.end(), rng);

    return std::async(std::launch::async, [this, ROWS, COLUMNS, shuffled_indices]() mutable {
        lock_guard<mutex> lock(m_cells_mutex);

        this->build_fut(cref(shuffled_indices));

        auto found = search(get_int(0, ROWS * COLUMNS - 1));
        m_distances = make_shared<distances>(found);
        m_distances = m_distances->dist();

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

std::shared_ptr<distances> distance_grid::get_distances() const noexcept {
	return this->m_distances;
}
