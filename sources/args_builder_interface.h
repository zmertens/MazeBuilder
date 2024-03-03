#ifndef ARGS_BUILDER_INTERFACE_H
#define ARGS_BUILDER_INTERFACE_H

#include <unordered_map>
#include <string>

class args_builder_interface {
public:
    virtual args_builder_interface& set_seed(unsigned int s) noexcept = 0;
    virtual args_builder_interface& set_interactive(bool i) noexcept = 0;
    virtual args_builder_interface& set_algo(const std::string& algo) noexcept = 0;
    virtual args_builder_interface& set_output(const std::string& filename) noexcept = 0;
    virtual const std::unordered_map<std::string, std::string>& build() const noexcept = 0;
};

#endif // ARGS_BUILDER_INTERFACE
