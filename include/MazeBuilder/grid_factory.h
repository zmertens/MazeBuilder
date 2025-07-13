#ifndef grid_factory_H
#define grid_factory_H

#include <functional>
#include <memory>
#include <random>
#include <optional>

#include <MazeBuilder/factory_interface.h>

namespace mazes {

class configurator;
class grid_interface;

/// @file grid_factory.h
/// @class grid_factory
/// @brief grid_factory provides a way to create grids with algorithms applied
class grid_factory : public factory_interface {

public:

	std::unique_ptr<grid_interface> create(configurator const& config) const noexcept override;

private:

	static std::optional<std::unique_ptr<grid_interface>> create_grid(configurator const& config) noexcept;
};

} // namespace mazes

#endif // grid_factory_H
