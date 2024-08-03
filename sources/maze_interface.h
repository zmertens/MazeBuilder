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
#include <cstdlib>
#include <random>
#include <functional>

#include "maze_types_enum.h"

namespace mazes {
class maze_interface {
public:
	virtual void set_maze(const std::string& maze) noexcept = 0;
	virtual std::string get_maze() noexcept = 0;
	virtual void clear() noexcept = 0;
	// The tuple holds <x, y, z, w> where w is the block type
	virtual std::vector<std::tuple<int, int, int, int>> get_render_vertices() const noexcept = 0;
	virtual std::vector<std::tuple<int, int, int, int>> get_writable_vertices() const noexcept = 0;
	virtual std::vector<std::vector<std::uint32_t>> get_faces() const noexcept = 0;
	// Get a 2D maze as a string
	virtual std::string compute_str(maze_types my_maze_type, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept = 0;
	// Compute the geometry of the maze - set vertex data - (includes height for 3D mazes)
	virtual void compute_geometry() noexcept = 0;
private:
	virtual void add_block(int x, int y, int z, int w, int block_size) noexcept = 0;
};
} // namespace

#endif // MAZE_INTERFACE_H
