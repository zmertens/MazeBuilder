#ifndef ARGS_HANDLER_H
#define ARGS_HANDLER_H

#include <string>
#include <sstream>
#include <unordered_map>
#include <ostream>
#include <vector>
#include <functional>

#include "args_builder_interface.h"

namespace mazes {

class args {
public:
    std::string output;
    std::string algorithm;
    unsigned int seed;
    unsigned int width;
    unsigned int length;
    unsigned int height;
    unsigned int cell_size;
    bool interactive;
    std::string version;
    std::string help;

    friend std::ostream& operator<<(std::ostream& os, args& a) {
        std::stringstream ss;
        ss << "seed=" << a.seed << "\n";
        ss << "interactive=" << a.interactive << "\n";
        ss << "algorithm=" << a.algorithm << "\n";
        ss << "output=" << a.output << "\n";
        ss << "width=" << a.width << "\n";
        ss << "length=" << a.length << "\n";
        ss << "height=" << a.height << "\n";
        ss << "cell_size=" << a.cell_size << "\n";
        ss << "help=" << a.help << "\n";
        ss << "version=" << a.version << "\n";

        return os << ss.str() << "\n";
    }
}; // class args

class args_builder : public args_builder_interface {
public:
    explicit args_builder(const std::vector<std::string>& vv);

    args_builder& seed(unsigned int seed) noexcept;
    args_builder& interactive(bool interactive) noexcept;
    args_builder& version(const std::string& version) noexcept;
    args_builder& help(const std::string& help) noexcept;
    args_builder& algorithm(const std::string& algorithm) noexcept;
    args_builder& output(const std::string& output) noexcept;
    args_builder& width(unsigned int width) noexcept;
    args_builder& length(unsigned int length) noexcept;
    args_builder& height(unsigned int height) noexcept;
    args_builder& cell_size(unsigned int cell_size) noexcept;

    void clear() noexcept;

    args build() const noexcept override;

private:
    void parse(const std::vector<std::string>& vv) noexcept override;
    args my_args;
};

}
#endif // ARGS_HANDLER_H
