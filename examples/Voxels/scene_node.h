#ifndef SCENE_NODE_H
#define SCENE_NODE_H

#include "entity.h"

// Base class for all game objects that can receive commands
class scene_node
{
public:
    scene_node() : m_category{Entity::SCENE} {}

    virtual ~scene_node() = default;

    scene_node(const scene_node&) = delete;
    scene_node& operator=(const scene_node&) = delete;

    scene_node(scene_node&&) noexcept = default;
    scene_node& operator=(scene_node&&) noexcept = default;

    [[nodiscard]] Entity get_category() const noexcept
    {
        return m_category;
    }

protected:
    void set_category(Entity category) noexcept
    {
        m_category = category;
    }

private:
    Entity m_category;
};

#endif // SCENE_NODE_H

