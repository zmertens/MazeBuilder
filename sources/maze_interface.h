/**
 * Allow library users to interact with the maze data.
 *
 */

#ifndef MAZE_INTERFACE_H
#define MAZE_INTERFACE_H

#include <vector>
#include <tuple>
#include <cstdlib>

namespace mazes {
class maze_interface {
public:
	virtual void set_maze(const std::string& maze, unsigned int height) noexcept = 0;
	virtual std::string get_maze() noexcept = 0;
	virtual void clear() noexcept = 0;
	virtual std::vector<std::tuple<int, int, int, int>> get_render_vertices() noexcept = 0;
	virtual std::vector<std::tuple<int, int, int, int>> get_wavefront_obj_vertices() noexcept = 0;
	virtual std::vector<std::vector<std::uint32_t>> get_faces() noexcept = 0;
	virtual void add_block(int x, int y, int z, int w, int block_size) noexcept = 0;
	virtual std::string to_wavefront_obj_str() const noexcept = 0;
private:
	virtual void compute_geometry() noexcept = 0;
};
} // namespace

#endif // MAZE_INTERFACE_H
