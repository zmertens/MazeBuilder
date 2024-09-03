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
        bool interactive;
        std::string version;
        std::string help;
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
    args_builder& state() noexcept;

    void clear() noexcept;

    const args& build() const noexcept override;


    friend std::ostream& operator<<(std::ostream& os, args_builder& args_builder) {
        std::stringstream ss;

        auto&& a = args_builder.build();

        ss << "INFO: seed=" << a.seed << "\n";
        ss << "INFO: interactive=" << a.interactive << "\n";
        ss << "INFO: algorithm=" << a.algorithm << "\n";
        ss << "INFO: output=" << a.output << "\n";
        ss << "INFO: width=" << a.width << "\n";
        ss << "INFO: length=" << a.length << "\n";
        ss << "INFO: help=" << a.help << "\n";
        ss << "INFO: version=" << a.version << "\n";

        return os << ss.str() << "\n";
    }

private:
    void parse(const std::vector<std::string>& vv) noexcept override;
    args my_args;
};

}
#endif // ARGS_HANDLER_H
