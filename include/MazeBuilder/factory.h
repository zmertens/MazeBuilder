#ifndef MAZE_FACTORY_H
#define MAZE_FACTORY_H

#include <functional>
#include <memory>
#include <random>
#include <optional>

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

	private:

	static bool run_algo_on_grid(algos a, std::unique_ptr<grid_interface>& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) noexcept;


};

} // namespace mazes

#endif // MAZE_FACTORY_H
