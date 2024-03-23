#ifndef ARGS_BUILDER_INTERFACE_H
#define ARGS_BUILDER_INTERFACE_H

#include <unordered_map>
#include <string>

class args_builder_interface {
public:
    virtual const std::unordered_map<std::string, std::string>& build() = 0;
};

#endif // ARGS_BUILDER_INTERFACE
