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

    // Web interaction
    [[nodiscard]] std::string artifacts() const noexcept;
    [[nodiscard]] bool is_download_ready() const noexcept;
    void reset_download_flag() const noexcept;

private:
    struct craft_impl;

    std::unique_ptr<craft_impl> m_impl;
};

#endif // CRAFT_H
