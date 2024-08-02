#ifndef MAZE_THREAD_SAFE_H
#define MAZE_THREAD_SAFE_H

#include <string>
#include <shared_mutex>
#include <mutex>

#include "maze_interface.h"

class maze_thread_safe : public mazes::maze_interface
{
public:
    maze_thread_safe(const std::string& maze, unsigned int height);
	virtual void set_maze(const std::string& maze, unsigned int height) noexcept override;
	virtual std::string get_maze() noexcept override;
	virtual void clear() noexcept override;
	virtual std::vector<std::tuple<int, int, int, int>> get_render_vertices() const noexcept override;
    virtual std::vector<std::tuple<int, int, int, int>> get_block_vertices() const noexcept override;
	virtual std::vector<std::vector<std::uint32_t>> get_faces() const noexcept override;
	void add_block(int x, int y, int z, int w, int block_size) noexcept override;
	std::string to_wavefront_obj_str() const noexcept override;

private:
    void compute_geometry() noexcept;

    std::string m_maze;
    unsigned int m_height;
    // Mutable allows for const methods to modify the object
    mutable std::shared_mutex m_verts_mtx;
    std::mutex m_maze_mutx;
    // Tuple (x, y, z, w)
    // Write vertices compute block sizes and are designed for Wavefront OBJ file writing
    std::vector<std::tuple<int, int, int, int>> m_vertices;
    std::vector<std::vector<std::uint32_t>> m_faces;
}; // class

#endif // MAZE_THREAD_SAFE_H

