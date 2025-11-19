#include "PhysicsContactListener.hpp"
#include "Entity.hpp"

#include <box2d/box2d.h>

// C-style Box2D contact callbacks that forward to Entity methods
bool PhysicsContactListener_BeginContact(b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Manifold* manifold, void* context)
{
    // Extract body user data to get Entity pointers
    const b2BodyId bodyIdA = b2Shape_GetBody(shapeIdA);
    const b2BodyId bodyIdB = b2Shape_GetBody(shapeIdB);

    if (!b2Body_IsValid(bodyIdA) || !b2Body_IsValid(bodyIdB))
    {
        return true;
    }

    if (auto* entityA = static_cast<Entity*>(b2Body_GetUserData(bodyIdA)))
    {
        if (auto* entityB = static_cast<Entity*>(b2Body_GetUserData(bodyIdB)))
        {
            entityA->onBeginContact(entityB);
            entityB->onBeginContact(entityA);
        }
    }

    return true;
}

void PhysicsContactListener_EndContact(b2ShapeId shapeIdA, b2ShapeId shapeIdB, void* context)
{
    const b2BodyId bodyIdA = b2Shape_GetBody(shapeIdA);
    const b2BodyId bodyIdB = b2Shape_GetBody(shapeIdB);

    if (!b2Body_IsValid(bodyIdA) || !b2Body_IsValid(bodyIdB))
    {
        return;
    }

    if (auto* entityA = static_cast<Entity*>(b2Body_GetUserData(bodyIdA)))
    {
        if (auto* entityB = static_cast<Entity*>(b2Body_GetUserData(bodyIdB)))
        {
            entityA->onEndContact(entityB);
            entityB->onEndContact(entityA);
        }
    }
}

bool PhysicsContactListener_PreSolve(b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Manifold* manifold, void* context)
{
    // Optional: can modify contact before solving
    return true;
}

