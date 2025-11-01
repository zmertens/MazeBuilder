#ifndef SCENE_NODE_HPP
#define SCENE_NODE_HPP

#include "Category.hpp"
#include "Transformable.hpp"
#include "RenderStates.hpp"

#include <memory>
#include <vector>
#include <box2d/math_functions.h>

struct Command;

class SceneNode : public Transformable {
public:
    using Ptr = std::unique_ptr<SceneNode>;

public:
    explicit SceneNode();
    virtual ~SceneNode() = default;

    // Delete copy constructor and copy assignment operator
    // because SceneNode contains std::unique_ptr which is not copyable
    SceneNode(const SceneNode&) = delete;
    SceneNode& operator=(const SceneNode&) = delete;

    // Allow move constructor and move assignment operator
    SceneNode(SceneNode&&) = default;
    SceneNode& operator=(SceneNode&&) = default;

    void attachChild(Ptr child);
    Ptr detachChild(const SceneNode& node);

    void update(float dt) noexcept;

    // Public draw method for Drawable interface (like sf::Drawable)
    void draw(RenderStates states) const noexcept;

    b2Vec2 getWorldPosition() const;
    Transformable getWorldTransform() const;

    void onCommand(const Command& command, float dt) noexcept;

    virtual Category::Type getCategory() const noexcept;

private:
    virtual void updateCurrent(float dt) noexcept;
    void updateChildren(float dt) noexcept;

    virtual void drawCurrent(RenderStates states) const noexcept;
    void drawChildren(RenderStates states) const noexcept;

private:
    std::vector<Ptr> mChildren;
    SceneNode* mParent;
};

#endif // SCENE_NODE_HPP
