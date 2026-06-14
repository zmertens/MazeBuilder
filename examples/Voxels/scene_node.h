#ifndef SCENE_NODE_H
#define SCENE_NODE_H

#include "entity.h"
#include "voxels_map.h"
#include "sign.h"

#include <vector>

// Base class for all game objects that can interact in the world
class scene_node
{
public:
    // Chunk-specific data
    voxels_map map;
    voxels_map lights;
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


    scene_node* parent;
    std::vector<scene_node*> children;

    // Spatial bounds (for quadtree nodes)
    int bounds_min_x;
    int bounds_min_z;
    int bounds_max_x;
    int bounds_max_z;

    scene_node() : map(), lights(), signs(), p(0), q(0), faces(0), sign_faces(0), dirty(0), miny(0), maxy(0), buffer(0),
                   sign_buffer(0),
                   parent{nullptr},
                   children{},
                   bounds_min_x{0}, bounds_min_z{0}, bounds_max_x{0}, bounds_max_z{0},
                   m_category{Entity::SCENE}
    {
    }

    virtual ~scene_node() = default;

    // Copy constructor: properly handles all members
    scene_node(const scene_node& other)
        : map(other.map)
        , lights(other.lights)
        , signs(other.signs)
        , p(other.p)
        , q(other.q)
        , faces(other.faces)
        , sign_faces(other.sign_faces)
        , dirty(other.dirty)
        , miny(other.miny)
        , maxy(other.maxy)
        , buffer(other.buffer)
        , sign_buffer(other.sign_buffer)
        , parent(nullptr)
        , children()
        , bounds_min_x(other.bounds_min_x)
        , bounds_min_z(other.bounds_min_z)
        , bounds_max_x(other.bounds_max_x)
        , bounds_max_z(other.bounds_max_z)
        , m_category(other.m_category)
    {
    }

    // Copy assignment operator: properly handles all members
    scene_node& operator=(const scene_node& other) {
        if (this != &other) {
            // Copy chunk-specific data
            map = other.map;
            lights = other.lights;
            signs = other.signs;
            p = other.p;
            q = other.q;
            faces = other.faces;
            sign_faces = other.sign_faces;
            dirty = other.dirty;
            miny = other.miny;
            maxy = other.maxy;
            buffer = other.buffer;
            sign_buffer = other.sign_buffer;

            // Copy spatial bounds
            bounds_min_x = other.bounds_min_x;
            bounds_min_z = other.bounds_min_z;
            bounds_max_x = other.bounds_max_x;
            bounds_max_z = other.bounds_max_z;

            // Copy category
            m_category = other.m_category;

            // Note: parent and children pointers are NOT copied
            // They need to be managed separately by the scene graph
            // parent remains as-is, children remains as-is
        }
        return *this;
    }

    // Move constructor and assignment: defaulted
    scene_node(scene_node&&) noexcept = default;
    scene_node& operator=(scene_node&&) noexcept = default;

    [[nodiscard]] Entity get_category() const noexcept
    {
        return m_category;
    }

    void set_category(const Entity category) noexcept
    {
        m_category = category;
    }

    // Check if this node or its children intersect with a 2D bounds (for frustum culling)
    [[nodiscard]] bool intersects_bounds(const int min_x, const int min_z,
        const int max_x, const int max_z) const noexcept
    {
        return !(bounds_max_x < min_x || bounds_min_x > max_x ||
                 bounds_max_z < min_z || bounds_min_z > max_z);
    }

private:
    Entity m_category;
};

#endif // SCENE_NODE_H

