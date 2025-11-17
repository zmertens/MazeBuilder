#include "SceneNode.hpp"

#include "Command.hpp"
#include "CommandQueue.hpp"
#include "RenderStates.hpp"

#include <box2d/math_functions.h>

#include <SDL3/SDL.h>
#include "SDLHelper.hpp"

#include <algorithm>
#include <cassert>

SceneNode::SceneNode()
    : mChildren{}, mParent(nullptr), mDefaultCategory(Category::Type::PICKUP)
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

void SceneNode::update(float dt, CommandQueue& commands) noexcept
{
    updateCurrent(dt, std::ref(commands));
    updateChildren(dt, std::ref(commands));
}

void SceneNode::updateCurrent(float, CommandQueue& commands) noexcept
{
    // Do nothing by default
}

void SceneNode::updateChildren(float dt, CommandQueue& commands) noexcept
{
    for (auto& child : mChildren)
    {
        child->update(dt, std::ref(commands));
    }
}

void SceneNode::draw(SDL_Renderer* renderer, RenderStates states) const noexcept
{
    states.transform.p.x += getPosition().x;
    states.transform.p.y += getPosition().y;

    states.transform.q = getRotation();

    drawCurrent(renderer, states);
    drawChildren(renderer, states);
}

void SceneNode::drawCurrent(SDL_Renderer*, RenderStates) const noexcept
{
    // Do nothing by default
}

void SceneNode::drawChildren(SDL_Renderer* renderer, RenderStates states) const noexcept
{
    for (auto& child : mChildren)
    {
        child->draw(renderer, states);
    }
}

b2Vec2 SceneNode::getWorldPosition() const
{
    return {getWorldTransform().getPosition().x, getWorldTransform().getPosition().y};
}

Transformable SceneNode::getWorldTransform() const
{
    b2Transform transform = b2Transform_identity;

    for (const SceneNode* node = this; node != nullptr; node = node->mParent)
    {
        b2Transform localTransform;
        localTransform.p = node->getPosition();
        localTransform.q = node->getRotation();
        transform = b2MulTransforms(localTransform, transform);
    }

    Transformable worldTransform;
    worldTransform.setPosition(transform.p);
    worldTransform.setRotation(transform.q);

    return worldTransform;
}

void SceneNode::onCommand(const Command& command, float dt) noexcept
{
    // Check if the command applies to this node using bitwise AND
    // This allows a single command to target multiple category types
    if (static_cast<unsigned int>(command.category) & static_cast<unsigned int>(getCategory()))
    {
        command.action(*this, dt);
    }

    // Pass the command to the children
    for (const auto& child : mChildren)
    {
        child->onCommand(command, dt);
    }
}

Category::Type SceneNode::getCategory() const noexcept
{
    return Category::Type::SCENE;
}
