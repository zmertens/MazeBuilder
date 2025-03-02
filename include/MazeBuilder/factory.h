#ifndef FACTORY_H
#define FACTORY_H

#include <functional>
#include <memory>
#include <random>
#include <optional>

#include <MazeBuilder/enums.h>
#include <MazeBuilder/configurator.h>

namespace mazes {

class grid_interface;
class maze;

/// @file factory.h
/// @class factory
/// @brief Factory provides a way to create mazes
class factory {

public:

	/// @brief Create a maze pointer
	/// @param config 
	/// @return 
	static std::optional<std::unique_ptr<maze>> create(configurator const& config) noexcept;

private:

	/// @brief Apply an algorithm to a grid
	/// @param config 
	/// @param g 
	/// @param get_int 
	/// @param rng 
	/// @return 
	static bool apply_algo_to_grid(configurator const& config, std::unique_ptr<grid_interface> const& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) noexcept;
};

} // namespace mazes

#endif // FACTORY_H
