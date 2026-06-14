#ifndef COMMAND_H
#define COMMAND_H

#include <cstdint>
#include <functional>
#include <queue>
#include <type_traits>

#include "entity.h"
#include "scene_node.h"
#include "MazeBuilder/randomizer.h"

namespace mazes {
    class randomizer;
}

struct command
{
    std::function<void(scene_node &, float, mazes::randomizer& rng)> action;
    Entity category;
};

using command_queue = std::queue<command>;

template <typename GameObject, typename Function>
std::function<void(scene_node &, float, mazes::randomizer&)> derived_action(Function fn)
{
    return [=](scene_node &node, float dt, mazes::randomizer& rng)
    {
        // Ensure that the cast is safe - check if scene_node is base of GameObject
        if constexpr (std::is_base_of_v<scene_node, GameObject>)
        {
            fn(static_cast<GameObject &>(node), dt, std::ref(rng));
        }
    };
}

#endif // COMMAND_H
