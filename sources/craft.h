#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <string_view>

#include "maze_algo_interface.h"
#include "maze_types_enum.h"

class grid;
class writer;

class craft {
public:
    craft(const std::string_view& window_name, const std::string_view& version, const std::string_view& help);
    ~craft();
    // craft(const craft& rhs);
    // craft& operator=(const craft& rhs);

    bool run(bool interactive = false) const noexcept;
private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;

};

#endif // CRAFT_H
