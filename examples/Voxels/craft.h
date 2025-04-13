#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <memory>
#include <functional>
#include <random>

#include <MazeBuilder/randomizer.h>

/// @brief Monolithic class to handle running a voxel engine
class craft {
public:
    craft(const std::string& title, const std::string& version, int w, int h);
    ~craft();

    bool run(mazes::randomizer& rng) const noexcept;
    
    // Web interaction
    std::string mazes() const noexcept;
    void toggle_mouse() const noexcept;

    template <typename... Args>
    /// @brief Static method to access the singleton instance of the craft class.
    static std::shared_ptr<craft> get_instance(Args&&... args) noexcept {
        static std::shared_ptr<craft> instance = std::make_shared<craft>(std::forward<Args>(args)...);
        return instance;
    }

private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;
    static constexpr auto ZACHS_GH_REPO = R"gh(https://github.com/zmertens/MazeBuilder)gh";
};

#endif // CRAFT_H
