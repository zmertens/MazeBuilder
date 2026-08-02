#ifndef CRAFT_H
#define CRAFT_H

#include <memory>
#include <string>

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/singleton_base.h>

/// @brief Monolithic class to handle running a voxel engine
class craft final : mazes::singleton_base<craft> {
    friend class singleton_base;
public:
    craft(const std::string& title, int w, int h);
    ~craft();

    bool run(mazes::grid_interface* g, mazes::randomizer& rng) const noexcept;

    // -----------------------------------------------------------------------
    // Synchronous legacy path (kept for backward compatibility).
    // -----------------------------------------------------------------------
    [[nodiscard]] std::string artifacts() const noexcept;
    [[nodiscard]] bool is_download_ready() const noexcept;
    void reset_download_flag() const noexcept;

    // -----------------------------------------------------------------------
    // Async export — non-blocking path for the web front-end.
    // JS calls begin_export(), polls is_export_ready(), then calls get_export().
    // -----------------------------------------------------------------------
    void begin_export() const noexcept;                     // kick off async OBJ generation
    [[nodiscard]] bool is_export_ready() const noexcept;   // non-blocking poll for JS
    [[nodiscard]] std::string get_export() noexcept;       // retrieve result once ready
    [[nodiscard]] std::string get_export_status() const noexcept; // "idle"|"running"|"ready"|"error"

    // -----------------------------------------------------------------------
    // Maze configuration — JS can set these before triggering begin_export().
    // -----------------------------------------------------------------------
    void set_maze_rows(int rows) noexcept;
    void set_maze_columns(int cols) noexcept;
    void set_maze_algo(const std::string& algo) noexcept;  // e.g. "dfs", "binary_tree"
    void set_maze_seed(int seed) noexcept;

    // -----------------------------------------------------------------------
    // Engine meta — useful for JS status overlays.
    // -----------------------------------------------------------------------
    [[nodiscard]] std::string get_version() const noexcept;

private:
    struct craft_impl;

    std::unique_ptr<craft_impl> m_impl;
};

#endif // CRAFT_H
