/**
 * Allow library users to interact with the maze operations.
 * Does not assume if maze is 2D/3D but supports operations for both.
 *
 *
 */

#ifndef MAZE_INTERFACE_H
#define MAZE_INTERFACE_H

#include <vector>
#include <tuple>
#include <optional>
#include <cstdint>
#include <random>
#include <functional>

#include "maze_types_enum.h"

namespace mazes {
class maze_interface {
public:
	virtual void clear() noexcept = 0;
	// The tuple holds <x, y, z, w> where w is the block type
	virtual std::vector<std::tuple<int, int, int, int>> get_render_vertices() const noexcept = 0;
	virtual std::vector<std::tuple<int, int, int, int>> get_writable_vertices() const noexcept = 0;
	virtual std::vector<std::vector<std::uint32_t>> get_faces() const noexcept = 0;
	// Get a 2D maze as a string
	virtual std::string to_str(maze_types my_maze_type, const std::function<int(int, int)>& get_int, const std::mt19937& rng, bool distances = false) const noexcept = 0;
	virtual std::vector<std::uint8_t> to_pixels(mazes::maze_types my_maze_type,
		const std::function<int(int, int)>& get_int,
		const std::mt19937& rng,
		const unsigned int cell_size = 3) const noexcept = 0;
	// Compute the 3D geometry of the maze (includes height for 3D mazes)
	virtual void compute_geometry(maze_types my_maze_type, const std::function<int(int, int)>& get_int, const std::mt19937& rng, int block_type = 1) noexcept = 0;
	virtual std::optional<std::tuple<int, int, int, int>> find_block(int p, int q) const noexcept = 0;
private:
	virtual void add_block(int x, int y, int z, int w, int block_size) noexcept = 0;
};
} // namespace

#endif // MAZE_INTERFACE_H
