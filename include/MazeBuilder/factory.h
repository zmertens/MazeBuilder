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
	/// @param rows 
	/// @param columns 
	/// @return 
	static std::optional<std::unique_ptr<maze>> create_q(unsigned int rows = 10, unsigned int columns = 10) noexcept;

	/// @brief Create a maze pointer
	/// @param config 
	/// @return 
	static std::optional<std::unique_ptr<maze>> create(configurator const& config) noexcept;

private:

    static std::optional<std::unique_ptr<grid_interface>> create_grid(configurator const& config) noexcept;
};

} // namespace mazes

#endif // FACTORY_H
