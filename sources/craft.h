#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <future>

#include "maze_algo_interface.h"
#include "maze_types_enum.h"

class grid;
class writer;

class craft : public mazes::maze_algo_interface {
public:
    craft(const std::string_view& window_name, std::function<std::future<bool>(mazes::maze_types)> maze_func, std::packaged_task<bool(const std::string& data)> task_writes);
    ~craft();
    // craft(const craft& rhs);
    // craft& operator=(const craft& rhs);
    bool run(std::unique_ptr<mazes::grid> const& _grid, std::function<int(int, int)> const& get_int, bool interactive = false) const noexcept override;
private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;

};

#endif // CRAFT_H
