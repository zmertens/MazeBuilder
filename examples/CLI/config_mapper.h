#ifndef CONFIG_MAPPER_H
#define CONFIG_MAPPER_H

#include <string>
#include <vector>

namespace mazes
{
    class configurator;
}

class config_mapper {

public:
    static bool map_args_to_config(std::vector<std::string> const& args, mazes::configurator& config);
};

#endif // CONFIG_MAPPER_H
