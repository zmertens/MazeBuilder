#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "Category.hpp"

#include <functional>

class SceneNode;

struct Command
{
    std::function<void(SceneNode&, float)> action;
    Category::Type category;
};

template <typename GameObject, typename Function>
std::function<void(SceneNode&, float)> derivedAction(Function fn)
{
    return [=](SceneNode& node, float dt)
    {
        // Ensure that the cast is safe
        if (auto derived = dynamic_cast<GameObject*>(&node))
        {
            fn(static_cast<GameObject&>(node), dt);
        }
    };
}

#endif // COMMAND_HPP
