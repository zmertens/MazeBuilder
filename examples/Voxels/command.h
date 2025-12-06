#ifndef COMMAND_H
#define COMMAND_H

#include <cstdint>
#include <functional>
#include <type_traits>

#include "entity.h"

class scene_node;

struct command
{
    std::function<void(scene_node &, float)> action;
    Entity category;
};

template <typename GameObject, typename Function>
std::function<void(scene_node &, float)> derived_action(Function fn)
{
    return [=](scene_node &node, float dt)
    {
        // Ensure that the cast is safe
        if constexpr (std::is_base_of_v<GameObject, scene_node>)
        {
            fn(static_cast<GameObject &>(node), dt);
        }
    };
}

#endif // COMMAND_H
