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

    friend class mazes::singleton_base<cli>;
public:

    std::string convert(std::vector<std::string> const& args_vec) const noexcept;

    std::string convert_as_base64(std::vector<std::string> const& args_vec) const noexcept;

    std::string help() const noexcept;

    std::string version() const noexcept;
    
    /// @brief Get the configuration from the last convert call
    /// @return The configuration object, or nullptr if no valid configuration exists
    std::shared_ptr<mazes::configurator> get_config() const noexcept;
    
private:

    void apply(std::unique_ptr<mazes::grid_interface> const& g, mazes::randomizer& rng, mazes::algo a, const mazes::configurator& config) const noexcept;

    static std::string debug_str;

    static std::string help_str;

    static std::string title_str;

    static std::string version_str;

    // Store the last configuration
    mutable std::shared_ptr<mazes::configurator> m_config;

};


#endif // CLI_H
