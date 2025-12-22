#ifndef CRAFT_H
#define CRAFT_H

#include <memory>
#include <string>

#include <MazeBuilder/algo_interface.h>
#include <MazeBuilder/singleton_base.h>

namespace mazes
{
    class grid_interface;
    class randomizer;
}

/// @brief Monolithic class to handle running a voxel engine
class craft final : public mazes::algo_interface, mazes::singleton_base<craft> {
    friend class singleton_base;
public:
    craft(const std::string& title, int w, int h);
    ~craft() override;

    bool run(mazes::grid_interface* g, mazes::randomizer& rng) const noexcept override;

    // Web interaction
    [[nodiscard]] std::string artifacts() const noexcept;
    [[nodiscard]] bool is_download_ready() const noexcept;
    void reset_download_flag() const noexcept;

private:
    struct craft_impl;

    std::unique_ptr<craft_impl> m_impl;
};

#endif // CRAFT_H
