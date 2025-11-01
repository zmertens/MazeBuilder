#include "SceneNode.hpp"
#include "RenderStates.hpp"

#include <box2d/math_functions.h>

#include <SDL3/SDL.h>
#include "SDLHelper.hpp"

#include <algorithm>
#include <cassert>

SceneNode::SceneNode()
    : mParent(nullptr)
{
}

void SceneNode::attachChild(Ptr child)
{
    child->mParent = this;
    mChildren.push_back(std::move(child));
}

SceneNode::Ptr SceneNode::detachChild(const SceneNode& node)
{
    auto found = std::find_if(mChildren.begin(), mChildren.end(), [&](Ptr& p) { return p.get() == &node; });
    assert(found != mChildren.end());

    Ptr result = std::move(*found);
    result->mParent = nullptr;
    mChildren.erase(found);
    return result;
}

void SceneNode::update(float dt) noexcept {
    updateCurrent(dt);
    updateChildren(dt);
}

void SceneNode::updateCurrent(float) noexcept
{
    // Do nothing by default
}

void SceneNode::updateChildren(float dt) noexcept
{
    for (auto& child : mChildren) {

        child->update(dt);
    }
}

void SceneNode::draw(RenderStates states) const noexcept
{
    b2Transform localTransform;
    localTransform.p = getPosition();
    localTransform.q = getRotation();
    
    // Manual transform accumulation to replace broken b2MulTransforms
    states.transform.p.x += localTransform.p.x;
    states.transform.p.y += localTransform.p.y;
    // For rotation: states.transform.q = b2MulRot(states.transform.q, localTransform.q);

    drawCurrent(states);
    drawChildren(states);
}

void SceneNode::drawCurrent(RenderStates) const noexcept
{
    // Do nothing by default
}

void SceneNode::drawChildren(RenderStates states) const noexcept
{
    for (auto& child : mChildren) {
        child->draw(states);
    }
}

b2Vec2 SceneNode::getWorldPosition() const
{
    return getWorldTransform().p;
}

b2Transform SceneNode::getWorldTransform() const
{
    b2Transform transform = b2Transform_identity;

    for (const SceneNode* node = this; node != nullptr; node = node->mParent)
    {
        b2Transform localTransform;
        localTransform.p = node->getPosition();
        localTransform.q = node->getRotation();
        transform = b2MulTransforms(localTransform, transform);
    }

    return transform;
}
