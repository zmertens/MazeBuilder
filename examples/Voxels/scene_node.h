#ifndef SCENE_NODE_H
#define SCENE_NODE_H

#include "entity.h"
#include "map.h"
#include "sign.h"

// Base class for all game objects that can interact in the world
class scene_node
{
public:
    Map map;
    Map lights;
    SignList signs;
    int p;
    int q;
    int faces;
    int sign_faces;
    int dirty;
    int miny;
    int maxy;
    std::uint32_t buffer;
    std::uint32_t sign_buffer;

    scene_node() : map(), lights(), signs(), p(0), q(0), faces(0), sign_faces(0), dirty(0), miny(0), maxy(0), buffer(0),
                   sign_buffer(0),
                   m_category{Entity::SCENE}
    {
    }

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
    void set_category(const Entity category) noexcept
    {
        m_category = category;
    }

private:
    Entity m_category;
};

#endif // SCENE_NODE_H

