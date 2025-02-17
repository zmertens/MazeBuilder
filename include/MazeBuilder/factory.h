#ifndef MAZE_FACTORY_H
#define MAZE_FACTORY_H

#include <functional>
#include <memory>
#include <random>
#include <optional>
#include <array>

#include <MazeBuilder/enums.h>

namespace mazes {

class grid_interface;
class maze;

/// @brief Factory provides a way to create mazes
class factory {

	public:

	static std::optional<std::unique_ptr<maze>> create(unsigned int rows, unsigned int columns, unsigned int height) noexcept;
	
	static std::optional<std::unique_ptr<maze>> create(std::tuple<unsigned int, unsigned int, unsigned int> dimensions) noexcept;
	
	static std::optional<std::unique_ptr<maze>> create(std::tuple<unsigned int, unsigned int, unsigned int> dimensions,
		algos a) noexcept;

	static std::optional<std::unique_ptr<maze>> create(std::tuple<unsigned int, unsigned int, unsigned int> dimensions,
		algos a, const std::function<int(int, int)>& get_int, const std::mt19937& rng) noexcept;

	static std::optional<std::unique_ptr<maze>> create(const std::vector<std::vector<bool>>& m) noexcept;

	private:

	static bool run_algo_on_grid(algos a, std::unique_ptr<grid_interface> const& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) noexcept;

    //static constexpr auto FACTORY_MAPPING_TOTAL = 5;
    //static std::array<std::function<std::optional<std::unique_ptr<maze>>>, FACTORY_MAPPING_TOTAL> factory_mappings;

};

} // namespace mazes

#endif // MAZE_FACTORY_H
