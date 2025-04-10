#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <memory>
#include <functional>
#include <random>

/// @brief Monolithic class to handle running a voxel engine
class craft {
public:
    craft(const std::string& title, const std::string& version, int w, int h);
    ~craft();

    // Delete copy constructor and copy assignment operator
    craft(const craft&) = delete;
    craft& operator=(const craft&) = delete;

    // Default move constructor and move assignment operator
    craft(craft&&) = default;
    craft& operator=(craft&&) = default;

    bool run(const std::function<int(int, int)>& get_int, std::mt19937& rng) const noexcept;
    
    // Web interaction
    std::string mazes() const noexcept;
    void toggle_mouse() const noexcept;
    
    // Singleton pattern
    static std::shared_ptr<craft> get_instance(const std::string& title, const std::string& version, int w, int h) {
        static std::shared_ptr<craft> instance = std::make_shared<craft>(cref(title), std::cref(version), w, h);
        return instance;
    }
private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;
    static constexpr auto ZACHS_GH_REPO = R"gh(https://github.com/zmertens/MazeBuilder)gh";
};

#endif // CRAFT_H
