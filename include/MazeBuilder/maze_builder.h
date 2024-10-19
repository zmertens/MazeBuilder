#ifndef MAZE_BUILDER_H
#define MAZE_BUILDER_H

#include <string>
#include <memory>
#include <utility>
#include <tuple>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include <mutex>
#include <chrono>
#include <functional>
#include <vector>
#include <random>

#include <MazeBuilder/maze_types_enum.h>
#include <MazeBuilder/grid_interface.h>

namespace mazes {

class maze_builder
{
    class cell;
private:
    // Store maze block's relative position in a grid / chunk-based world
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            auto hash1 = std::hash<T1>{}(std::get<0>(p));
            auto hash2 = std::hash<T1>{}(std::get<1>(p));
            return hash1 ^ hash2;
        }
    };

    using pqmap = std::unordered_map<std::pair<int, int>, std::tuple<int, int, int, int>, pair_hash>;

public:
    using maze = std::tuple<int, int, int, int, std::string, std::string>;

    // Constructor
    explicit maze_builder(int rows, int cols, int height, bool show_distances = false, int block_type = 1);
    explicit maze_builder(int rows, int cols, int height,
        mazes::maze_types my_maze_type,
        const std::function<int(int, int)>& get_int,
        const std::mt19937& rng,
        bool show_distances = false,
        int block_type = 1);

	void clear() noexcept;
	std::vector<std::tuple<int, int, int, int>> get_render_vertices() const noexcept;
    std::vector<std::tuple<int, int, int, int>> get_writable_vertices() const noexcept;
	std::vector<std::vector<std::uint32_t>> get_faces() const noexcept;

    std::optional<std::tuple<int, int, int, int>> find_block(int p, int q) const noexcept;

    std::string to_str() const noexcept;

    std::string to_str64() const noexcept;
    
    std::vector<std::uint8_t> to_pixels(const unsigned int cell_size = 3) const noexcept;

    std::string to_json_str(unsigned int pretty_spaces = 4) const noexcept;

    std::string to_wavefront_obj_str() const noexcept;
    
    int get_height() const noexcept;
    int get_columns() const noexcept;
    int get_rows() const noexcept;

    // Expose progress_tracker methods
    void start_progress() noexcept;
    void stop_progress() noexcept;
    double get_progress_in_seconds() const noexcept;
    double get_progress_in_ms() const noexcept;
    std::size_t get_vertices_size() const noexcept;

private:
    class progress_tracker {
        mutable std::mutex m_mtx;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point end_time;
    public:
        explicit progress_tracker() : start_time(std::chrono::steady_clock::now())
            , end_time(std::chrono::steady_clock::now()) {

        }
        void start() noexcept {
            this->m_mtx.lock();
            start_time = std::chrono::steady_clock::now();
            this->m_mtx.unlock();
        }

        void stop() noexcept {
            this->m_mtx.lock();
            end_time = std::chrono::steady_clock::now();
            this->m_mtx.unlock();
        }

        double get_duration_in_seconds() const noexcept {
            std::lock_guard<std::mutex> lock(this->m_mtx);
            return std::chrono::duration<double>(end_time - start_time).count();
        }

        double get_duration_in_ms() const noexcept {
            return this->get_duration_in_seconds() * 1000.0;
        }
    };

    void compute_geometry(mazes::maze_types my_maze_type, const std::function<int(int, int)>& get_int, const std::mt19937& rng) noexcept;
    void add_block(int x, int y, int z, int w, int block_size) noexcept;

    std::unique_ptr<grid_interface> m_grid;
    maze_types m_maze_type;
    int m_seed;
    bool m_show_distances;
    int m_block_type;

    // Tuple (x, y, z, block_type)
    std::vector<std::tuple<int, int, int, int>> m_vertices;
    std::vector<std::vector<std::uint32_t>> m_faces;
    pqmap m_p_q;

    progress_tracker m_tracker;
}; // class

} // namespace

#endif // MAZE_BUILDER_H

