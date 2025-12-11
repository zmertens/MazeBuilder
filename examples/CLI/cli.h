/// @file cli.h
/// @class cli
/// @brief Command-line interface for the maze builder application
/// @details This class provides a way to generate mazes by parsing command line arguments

#ifndef CLI_H
#define CLI_H

#include <memory>
#include <string>
#include <vector>

#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/singleton_base.h>

namespace mazes {

    class configurator;
}

class cli final :  mazes::singleton_base<cli> {

    friend class singleton_base<cli>;
public:
    std::string convert(std::vector<std::string> const& args_vec) const noexcept;

    static std::string convert(std::vector<std::string> const& args_vec,
                               mazes::configurator& user_options) noexcept;

    std::string convert_as_base64(std::vector<std::string> const& args_vec) const noexcept;

    std::string help() noexcept;

    std::string version() noexcept;

    // Helper: copy raw pixel bytes into a std::string for transport/storage
    static std::string bytes_to_string(const std::vector<std::uint8_t>& bytes);

    // Reverse helper: reconstruct a vector<uint8_t> from a raw bytes string
    static std::vector<std::uint8_t> string_to_bytes(const std::string& s);
private:
    static void apply(mazes::grid_interface* g,
                      mazes::randomizer& rng,
                      mazes::algo a,
                      const mazes::configurator& config) noexcept;

    static void compute_and_store_image_size(const mazes::grid_interface* g, mazes::configurator& cfg) noexcept;

    static std::string m_debug_str;

    static std::string m_help_str;

    static std::string m_title_str;

    static std::string m_version_str;

};


#endif // CLI_H
