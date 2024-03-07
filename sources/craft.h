#ifndef CRAFT_H
#define CRAFT_H

#include <string_view>
#include <functional>
#include <memory>
#include <list>

#include <glad/glad.h>

#include <SDL3/SDL.h>

#include <tinycthread/tinycthread.h>

#include "sign.h"
#include "map.h"
#include "config.h"

#include "maze_algo_interface.h"
#include "maze_types_enum.h"


class grid;

class craft : public mazes::maze_algo_interface {
public:
    craft(const std::string_view& window_name, std::future<bool> maze_future);
    ~craft();
    // craft(const craft& rhs);
    // craft& operator=(const craft& rhs);
    
    bool run(mazes::grid_ptr& gr, std::function<int(int, int)> const& get_int, bool interactive = false) noexcept override;
private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;

};

#endif // CRAFT_H
