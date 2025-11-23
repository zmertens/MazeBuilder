#ifndef CRAFT_H
#define CRAFT_H

#include <functional>
#include <memory>
#include <random>
#include <string>

#include <MazeBuilder/singleton_base.h>

namespace mazes
{
    class randomizer;
}

/// @brief Monolithic class to handle running a voxel engine
class craft final : public mazes::singleton_base<craft> {
    friend class singleton_base;
public:
    craft(const std::string& title, int w, int h);
    ~craft() override;

    bool run(mazes::randomizer& rng) const noexcept;

    // Web interaction
    [[nodiscard]] std::string mazes() const noexcept;
    void toggle_mouse() const noexcept;

    /// @brief Static method to access the singleton instance of the craft class.
    static std::shared_ptr<craft> get_instance(const std::string& title, int w, int h) noexcept {
        static auto instance = std::make_shared<craft>(std::cref(title), w, h);
        return instance;
    }

private:
    struct craft_impl;

    std::unique_ptr<craft_impl> m_impl;

    static constexpr auto MY_GITHUB_REPO = R"gh(https://github.com/zmertens/MazeBuilder)gh";
};

#endif // CRAFT_H
