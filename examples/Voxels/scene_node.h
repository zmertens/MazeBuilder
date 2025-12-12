#ifndef SCENE_NODE_H
#define SCENE_NODE_H

#include "entity.h"
#include "map.h"
#include "sign.h"
#include <vector>
#include <memory>

// Node types in the scene hierarchy
enum class SceneNodeType
{
    ROOT,       // Root of the scene graph
    LAYER,      // Layer node (background, foreground, etc.)
    CHUNK,      // Voxel chunk
    SPATIAL     // Spatial partitioning node (for quadtree)
};

// Base class for all game objects that can interact in the world
class scene_node
{
public:
    // Chunk-specific data
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

    // Hierarchy data
    SceneNodeType node_type;
    scene_node* parent;
    std::vector<scene_node*> children;

    // Spatial bounds (for quadtree nodes)
    int bounds_min_x;
    int bounds_min_z;
    int bounds_max_x;
    int bounds_max_z;

    scene_node() : map(), lights(), signs(), p(0), q(0), faces(0), sign_faces(0), dirty(0), miny(0), maxy(0), buffer(0),
                   sign_buffer(0),
                   node_type{SceneNodeType::CHUNK},
                   parent{nullptr},
                   children{},
                   bounds_min_x{0}, bounds_min_z{0}, bounds_max_x{0}, bounds_max_z{0},
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

    // Check if this node or its children intersect with a 2D bounds (for frustum culling)
    [[nodiscard]] bool intersects_bounds(int min_x, int min_z, int max_x, int max_z) const noexcept
    {
        return !(bounds_max_x < min_x || bounds_min_x > max_x ||
                 bounds_max_z < min_z || bounds_min_z > max_z);
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

