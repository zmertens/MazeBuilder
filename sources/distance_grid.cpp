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
bool distance_grid::get_future() noexcept {
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

    //return std::async(std::launch::async, [this, shuffled_indices]() mutable {
        this->build_fut(cref(shuffled_indices));

        //m_distances = make_shared<distances>(nullptr)->dist();

        return true;
        //});
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
