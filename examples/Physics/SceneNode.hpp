#ifndef SCENE_NODE_HPP
#define SCENE_NODE_HPP

#include "Drawable.hpp"
#include "Transformable.hpp"
#include "RenderStates.hpp"

#include <memory>
#include <vector>
#include <box2d/math_functions.h>

class SceneNode : public Transformable {
public:
    using Ptr = std::unique_ptr<SceneNode>;

public:
    explicit SceneNode();

    void attachChild(Ptr child);
    Ptr detachChild(const SceneNode& node);

    void update(float dt);

    b2Vec2 getWorldPosition() const;
    b2Transform getWorldTransform() const;

    void draw(RenderStates states) const noexcept;

private:
    virtual void updateCurrent(float dt);
    void updateChildren(float dt);

    virtual void drawCurrent(RenderStates states) const noexcept;
    void drawChildren(RenderStates states) const noexcept;

private:
    std::vector<Ptr> mChildren;
    SceneNode* mParent;
};

#endif // SCENE_NODE_HPP
