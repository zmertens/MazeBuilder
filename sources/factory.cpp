#include <MazeBuilder/factory.h>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/stringify.h>

#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace mazes;

factory::factory() {

    register_default_creators();
}

bool factory::register_creator(const std::string& key, grid_creator_t creator) {
    if (key.empty() || !creator) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_creators_mutex);
    
    // Check if key already exists
    if (m_creators.find(key) != m_creators.end()) {
        return false;
    }

    m_creators[key] = std::move(creator);
    return true;
}

bool factory::unregister_creator(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_creators_mutex);
    
    auto it = m_creators.find(key);
    if (it != m_creators.end()) {
        m_creators.erase(it);
        return true;
    }
    return false;
}

bool factory::is_registered(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_creators_mutex);
    return m_creators.find(key) != m_creators.end();
}

std::unique_ptr<grid_interface> factory::create(const std::string& key, const configurator& config) const {
    std::lock_guard<std::mutex> lock(m_creators_mutex);
    
    auto it = m_creators.find(key);
    if (it == m_creators.end()) {
        return nullptr;
    }

    try {
        auto grid = it->second(config);
        
        if (!grid) {
            return nullptr;
        }

        // Set up randomizer
        randomizer rng;
        rng.seed(config.seed());

        // Get dimensions from grid
        auto&& ops = grid->operations();

        // Generate indices for configuration  
        auto indices = rng.get_vector_ints(0, config.rows() * config.columns(), config.rows() * config.columns() - 1);

        // Prepare cells without any links - the maze algorithm will create the links
        std::vector<std::shared_ptr<cell>> cells_to_set;
        cells_to_set.reserve(config.rows() * config.columns());

        // Create cells with sequential indices (no neighbor links yet)
        for (size_t i = 0; i < config.rows() * config.columns(); ++i) {
            cells_to_set.push_back(std::make_shared<cell>(static_cast<int32_t>(i)));
        }

        // Set the cells in the grid - this will set up neighbor topology but no links
        ops.set_cells(std::cref(cells_to_set));

        return grid;
    }
    catch (const std::exception&) {
        // Log error in debug mode
#if defined(MAZE_DEBUG)
        std::cerr << "Error: Failed to create grid with key: " << key << std::endl;
#endif
        return nullptr;
    }
}

std::unique_ptr<grid_interface> factory::create(const configurator& config) const noexcept {

    try {

        // Determine the appropriate grid type based on configuration
        std::string grid_type = determine_grid_type_from_config(config);
        
        return create(grid_type, config);
    }
    catch (const std::exception&) {
        // In case of any error, return nullptr for backwards compatibility
#if defined(MAZE_DEBUG)
        std::cerr << "Error: Failed to create grid using default logic" << std::endl;
#endif
        return nullptr;
    }
}

std::vector<std::string> factory::get_registered_keys() const {
    std::lock_guard<std::mutex> lock(m_creators_mutex);
    
    std::vector<std::string> keys;
    keys.reserve(m_creators.size());
    
    for (const auto& pair : m_creators) {
        keys.push_back(pair.first);
    }
    
    return keys;
}

void factory::clear() {
    std::lock_guard<std::mutex> lock(m_creators_mutex);
    m_creators.clear();
    
    // Re-register defaults after clearing
    register_default_creators();
}

void factory::register_default_creators() {
    // Note: This method is called from constructor and clear(), 
    // so we don't need to lock here as the caller handles it or it's during construction

    // Register basic grid creator
    m_creators["grid"] = [](const configurator& config) -> std::unique_ptr<grid_interface> {
        return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
    };

    // Register distance grid creator
    m_creators["distance_grid"] = [](const configurator& config) -> std::unique_ptr<grid_interface> {
        return std::make_unique<distance_grid>(config.rows(), config.columns(), config.levels());
    };

    // Register colored grid creator
    m_creators["colored_grid"] = [](const configurator& config) -> std::unique_ptr<grid_interface> {
        return std::make_unique<colored_grid>(config.rows(), config.columns(), config.levels());
    };

    // Register convenience creators based on output_format type
    m_creators["image_grid"] = [](const configurator& config) -> std::unique_ptr<grid_interface> {
        if (config.distances()) {
            return std::make_unique<colored_grid>(config.rows(), config.columns(), config.levels());
        } else {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        }
    };

    m_creators["text_grid"] = [](const configurator& config) -> std::unique_ptr<grid_interface> {
        if (config.distances()) {
            return std::make_unique<distance_grid>(config.rows(), config.columns(), config.levels());
        } else {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        }
    };
}

std::string factory::determine_grid_type_from_config(const configurator& config) const {

    if (config.distances()) {

        if (config.output_format_id() == output_format::PNG || config.output_format_id() == output_format::JPEG) {

            return "colored_grid";
        } else {

            return "distance_grid";
        }
    } else {

        return "grid";
    }
}

