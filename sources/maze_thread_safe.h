#ifndef MAZE_THREAD_SAFE_H
#define MAZE_THREAD_SAFE_H

#include <string>
#include <shared_mutex>
#include <mutex>
#include <memory>
#include <utility>
#include <unordered_map>

#include "maze_interface.h"
#include "maze_types_enum.h"

class maze_thread_safe : public mazes::maze_interface
{
private:
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& pair) const {
            auto hash1 = std::hash<T1>{}(pair.first);
            auto hash2 = std::hash<T2>{}(pair.second);
            return hash1 ^ hash2;
        }
    };

    using pqmap = std::unordered_map<std::pair<int, int>, bool, pair_hash>;

public:
    maze_thread_safe(mazes::maze_types my_maze_type, const std::function<int(int, int)>& get_int, const std::mt19937& rng,
        unsigned int width, unsigned int length, unsigned int height);
	virtual void set_maze(const std::string& maze) noexcept override;
	virtual std::string get_maze() noexcept override;
	virtual void clear() noexcept override;
	virtual std::vector<std::tuple<int, int, int, int>> get_render_vertices() const noexcept override;
    virtual std::vector<std::tuple<int, int, int, int>> get_writable_vertices() const noexcept override;
	virtual std::vector<std::vector<std::uint32_t>> get_faces() const noexcept override;
    const pqmap& get_p_q() const noexcept;
    std::string compute_str(mazes::maze_types my_maze_type, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
    void compute_geometry() noexcept override;
    std::string to_wavefront_obj_str() const noexcept;

    void set_height(unsigned int height) noexcept;
    unsigned int get_height() const noexcept;
    void set_length(unsigned int length) noexcept;
    unsigned int get_length() const noexcept;
    void set_width(unsigned int width) noexcept;
    unsigned int get_width() const noexcept;

private:

    void add_block(int x, int y, int z, int w, int block_size) noexcept override;

    std::string m_maze;
    unsigned int m_width, m_length, m_height;
    // Mutable allows for const methods to modify the object
    mutable std::shared_mutex m_verts_mtx;
    std::mutex m_maze_mutx;
    // Tuple (x, y, z, w)
    // Write vertices compute block sizes and are designed for Wavefront OBJ file writing
    std::vector<std::tuple<int, int, int, int>> m_vertices;
    std::vector<std::vector<std::uint32_t>> m_faces;
    pqmap m_p_q;
}; // class

#endif // MAZE_THREAD_SAFE_H

