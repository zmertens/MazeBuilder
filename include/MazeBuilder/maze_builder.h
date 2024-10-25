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

class maze_builder {
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

    static const std::string MAZE_BUILDER_VERSION_STR;

public:
    // Data class to represent a maze
    struct maze {
        int rows, columns, height;
        mazes::maze_types maze_type;
        std::mt19937 rng;
        std::function<int(int, int)> get_int;
        int seed;
        bool show_distances;
        int block_type;
        int offset_x, offset_z;

        explicit maze() : rows(100), columns(100), height(0)
            , maze_type(mazes::maze_types::BINARY_TREE)
            , seed(0)
            , rng(std::mt19937(0))
            , show_distances(false), block_type(-1)
            , offset_x(0), offset_z(0) {
            this->get_int = { [this](int low, int high) {
                std::uniform_int_distribution<int> dist{ low, high };
                return dist(this->rng);
                } };
            // Constructor
        }

        std::vector<std::tuple<int, int, int, int>> get_render_vertices() const noexcept;
        std::vector<std::tuple<int, int, int, int>> get_writable_vertices() const noexcept;
        std::vector<std::vector<std::uint32_t>> get_faces() const noexcept;

        std::optional<std::tuple<int, int, int, int>> find_block(int x, int z) const noexcept;

        std::string to_str() const noexcept;

        std::string to_str64() const noexcept;

        std::vector<std::uint8_t> to_pixels(const unsigned int cell_size = 3) const noexcept;

        std::string to_json_str(unsigned int pretty_spaces = 4) const noexcept;

        std::string to_wavefront_obj_str64() const noexcept;
        std::string to_wavefront_obj_str() const noexcept;

        // Expose progress_tracker methods
        double get_progress_in_seconds() const noexcept;
        double get_progress_in_ms() const noexcept;

        // Compute 3D geometries and add blocks (includes height)
        void compute_geometry() noexcept;
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
        }; // progress_tracker
        
        void add_block(int x, int y, int z, int w, int block_size) noexcept;

        // Tuple (x, y, z, block_type)
        std::vector<std::tuple<int, int, int, int>> m_vertices;
        std::vector<std::vector<std::uint32_t>> m_faces;
        pqmap m_p_q;
        progress_tracker m_tracker;
        std::unique_ptr<grid_interface> m_grid;
    }; // maze struct

private:
    void start_progress() noexcept;
    void stop_progress() noexcept;
    std::unique_ptr<maze> my_maze;
public:
    explicit maze_builder() : my_maze(std::make_unique<maze>()) {

    }

    maze_builder& rows(int rows) {
        my_maze->rows = rows;
        return *this;
    }

    maze_builder& columns(int columns) {
        my_maze->columns = columns;
        return *this;
    }

    maze_builder& height(int height) {
        my_maze->height = height;
        return *this;
    }

    maze_builder& rng(std::mt19937 rng) {
        my_maze->rng = rng;
        return *this;
    }

    maze_builder& get_int(std::function<int(int, int)> get_int) {
        my_maze->get_int = get_int;
        return *this;
    }

    maze_builder& seed(int seed) {
        my_maze->seed = seed;
        return *this;
    }

    maze_builder& block_type(int block_type) {
        my_maze->block_type = block_type;
        return *this;
    }

    maze_builder& maze_type(mazes::maze_types maze_type) {
        my_maze->maze_type = maze_type;
        return *this;
    }

    maze_builder& show_distances(bool show_distances) {
        my_maze->show_distances = show_distances;
        return *this;
    }

    maze_builder& offset_x(int s_x) {
        my_maze->offset_x = s_x;
        return *this;
    }

    maze_builder& offset_z(int s_y) {
        my_maze->offset_z = s_y;
        return *this;
    }

    std::unique_ptr<maze> build() noexcept;
}; // class

} // namespace

#endif // MAZE_BUILDER_H

