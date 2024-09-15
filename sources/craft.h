#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <memory>
#include <functional>
#include <list>
#include <random>

#include "maze_types_enum.h"
#include "args_builder.h"

class craft {
public:
    craft(const std::string& version, const std::string& help, int w, int h);
    ~craft();

    // Delete copy constructor and copy assignment operator
    craft(const craft&) = delete;
    craft& operator=(const craft&) = delete;

    // Default move constructor and move assignment operator
    craft(craft&&) = default;
    craft& operator=(craft&&) = default;

    bool run(const std::list<std::string>& algos, 
        const std::function<mazes::maze_types(const std::string& algo)>& get_maze_type_from_str,
        const std::function<int(int, int)>& get_int, std::mt19937& rng) const noexcept;
    
    void set_json(const std::string& s) noexcept;
    std::string get_json() const noexcept;
    
    // Singleton pattern
    static std::shared_ptr<craft> get_instance(const std::string& version, const std::string& help, int w, int h) {
        static std::shared_ptr<craft> instance = std::make_shared<craft>(std::cref(version), std::cref(help), w, h);
        return instance;
    }
private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;
    static constexpr auto ZACHS_GH_REPO = R"gh(https://github.com/zmertens/MazeBuilder)gh";
};

#endif // CRAFT_H
