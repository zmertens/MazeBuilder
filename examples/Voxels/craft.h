#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <memory>
#include <functional>
#include <random>

#include <MazeBuilder/singleton_base.h>
#include <MazeBuilder/randomizer.h>

/// @brief Monolithic class to handle running a voxel engine
class craft : public mazes::singleton_base<craft> {

    friend class mazes::singleton_base<craft>;
public:
    craft(const std::string& title, const std::string& version, int w, int h);
    ~craft();

    bool run(mazes::randomizer& rng) const noexcept;
    
    // Web interaction
    std::string mazes() const noexcept;
    void toggle_mouse() const noexcept;

private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;
    static constexpr auto ZACHS_GH_REPO = R"gh(https://github.com/zmertens/MazeBuilder)gh";
};

#endif // CRAFT_H
