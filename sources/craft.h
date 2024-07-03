#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <string_view>
#include <memory>

class craft {
public:
    craft(const std::string_view& window_name, const std::string_view& version, const std::string_view& help);
    ~craft();

    // Delete copy constructor and copy assignment operator
    craft(const craft&) = delete;
    craft& operator=(const craft&) = delete;

    // Default move constructor and move assignment operator
    craft(craft&&) = default;
    craft& operator=(craft&&) = default;

    bool run() const noexcept;
private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;

};

#endif // CRAFT_H
