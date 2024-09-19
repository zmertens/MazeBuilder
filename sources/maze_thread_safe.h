#ifndef MAZE_THREAD_SAFE_H
#define MAZE_THREAD_SAFE_H

#include <string>
#include <shared_mutex>
#include <mutex>
#include <memory>
#include <utility>
#include <unordered_map>
#include <cstdint>

#include "maze_interface.h"
#include "maze_types_enum.h"

namespace mazes {

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
    explicit maze_thread_safe(unsigned int width, unsigned int length, unsigned int height);
	
	virtual void clear() noexcept override;
	virtual std::vector<std::tuple<int, int, int, int>> get_render_vertices() const noexcept override;
    virtual std::vector<std::tuple<int, int, int, int>> get_writable_vertices() const noexcept override;
	virtual std::vector<std::vector<std::uint32_t>> get_faces() const noexcept override;
    const pqmap& get_p_q() const noexcept;
    std::string to_str(mazes::maze_types my_maze_type, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
    void compute_geometry(mazes::maze_types my_maze_type, const std::function<int(int, int)>& get_int, const std::mt19937& rng, int block_type = 1) noexcept override;
    std::string to_wavefront_obj_str() const noexcept;
    std::vector<std::uint8_t> to_pixels(mazes::maze_types my_maze_type,
        const std::function<int(int, int)>& get_int,
        const std::mt19937& rng,
        const unsigned int cell_size = 3) const noexcept;

    void set_height(unsigned int height) noexcept;
    unsigned int get_height() const noexcept;
    void set_length(unsigned int length) noexcept;
    unsigned int get_length() const noexcept;
    void set_width(unsigned int width) noexcept;
    unsigned int get_width() const noexcept;
    void set_block_type(unsigned int block_type) noexcept;
    unsigned int get_block_type() const noexcept;

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


    void add_block(int x, int y, int z, int w, int block_size) noexcept override;

    std::string m_maze;
    unsigned int m_width, m_length, m_height;
    unsigned int m_block_type;
    // Mutable allows for const methods to modify the object
    mutable std::shared_mutex m_verts_mtx;
    std::mutex m_maze_mutx;
    // Tuple (x, y, z, w)
    // Write vertices compute block sizes and are designed for Wavefront OBJ file writing
    std::vector<std::tuple<int, int, int, int>> m_vertices;
    std::vector<std::vector<std::uint32_t>> m_faces;
    pqmap m_p_q;

    progress_tracker m_tracker;
}; // class

} // namespace

#endif // MAZE_THREAD_SAFE_H

