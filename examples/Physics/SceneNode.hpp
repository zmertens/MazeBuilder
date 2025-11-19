#ifndef SCENE_NODE_HPP
#define SCENE_NODE_HPP

#include "Category.hpp"
#include "Transformable.hpp"
#include "RenderStates.hpp"

#include <memory>
#include <vector>
#include <box2d/math_functions.h>

class CommandQueue;
struct Command;
struct SDL_Renderer;

class SceneNode : public Transformable
{
public:
    using Ptr = std::unique_ptr<SceneNode>;

public:
    explicit SceneNode();
    virtual ~SceneNode() = default;

    SceneNode(const SceneNode&) = delete;
    SceneNode& operator=(const SceneNode&) = delete;

    SceneNode(SceneNode&&) = default;
    SceneNode& operator=(SceneNode&&) = default;

    void attachChild(Ptr child);
    Ptr detachChild(const SceneNode& node);

    void update(float dt, CommandQueue& commands) noexcept;

    // Public draw method for Drawable interface (like sf::Drawable)
    void draw(SDL_Renderer* renderer, RenderStates states) const noexcept;

    b2Vec2 getWorldPosition() const;
    Transformable getWorldTransform() const;

    void onCommand(const Command& command, float dt) noexcept;

    virtual Category::Type getCategory() const noexcept;

private:
    virtual void updateCurrent(float dt, CommandQueue& commands) noexcept;
    void updateChildren(float dt, CommandQueue& commands) noexcept;

    virtual void drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept;
    void drawChildren(SDL_Renderer* renderer, RenderStates states) const noexcept;

    std::vector<Ptr> mChildren;
    SceneNode* mParent;

    Category::Type mDefaultCategory;
};

#endif // SCENE_NODE_HPP
