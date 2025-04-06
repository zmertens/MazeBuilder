#ifndef DISTANCE_GRID_H
#define DISTANCE_GRID_H

#include <future>
#include <memory>
#include <string>
#include <unordered_map>

#include <MazeBuilder/grid.h>

namespace mazes {

class distances;
class cell;

/// @file distance_grid.h
/// @class distance_grid
/// @brief A grid that can calculate distances between cells
class distance_grid : public grid {
public:
    friend class binary_tree;
    friend class dfs;
    friend class sidewinder;

	explicit distance_grid(unsigned int width = 1u, unsigned int length = 1u, unsigned int levels = 1u);
		
    virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept override;
    virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept override;

    std::shared_ptr<distances> get_distances() const noexcept;

    virtual std::future<bool> get_future() noexcept override;
private:
	std::shared_ptr<distances> m_distances;

	std::optional<std::string> to_base36(int value) const;
};
}

#endif // DISTANCE_GRID_H
