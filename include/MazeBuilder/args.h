#ifndef ARGS_H
#define ARGS_H

#include <string>
#include <sstream>
#include <unordered_map>
#include <ostream>
#include <vector>
#include <functional>

namespace mazes {

/// @brief Simple argument handler
struct args {
public:
    explicit args();

    friend std::ostream& operator<<(std::ostream& os, const args& a) {
        std::stringstream ss;
        ss << "\nINFO: seed=" << a.seed << "\n";
        ss << "algo=" << a.algo << "\n";
        ss << "output=" << a.output << "\n";
        ss << "columns=" << a.columns << "\n";
        ss << "rows=" << a.rows << "\n";
        ss << "height=" << a.height << "\n";
        ss << "help=" << a.help << "\n";
        ss << "version=" << a.version << "\n";
        ss << "distances=" << a.distances << "\n";

        return os << ss.str() << "\n";
    }

    /// @brief Parse program arguments
    /// @example auto args = mazes::args{};
    /// @param arg 
    /// @return 
    bool parse(const std::vector<std::string>& arg) noexcept;
public:
    std::string algo;
    bool distances;
    std::string output;
    int seed;
    int columns;
    int rows;
    int height;
    std::string version;
    std::string help;
};

}
#endif // ARGS_H
