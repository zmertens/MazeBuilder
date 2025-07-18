/// @file cli.h
/// @class cli
/// @brief Command-line interface for the maze builder application
/// @details This class implements the algo_interface and provides a way to run maze generation algorithms

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
    
    /// @brief Get the configuration from the last convert call
    /// @return The configuration object, or nullptr if no valid configuration exists
    std::shared_ptr<mazes::configurator> get_config() const noexcept;
    
private:

    void apply(std::unique_ptr<mazes::grid_interface> const& g, mazes::randomizer& rng, mazes::algo a, const mazes::configurator& config) const noexcept;

    static const std::string DEBUG_STR;

    static std::string CLI_HELP_STR;

    static std::string CLI_TITLE_STR;

    static std::string CLI_VERSION_STR;

    // Store the last configuration
    mutable std::shared_ptr<mazes::configurator> m_last_config;

};


#endif // CLI_H
