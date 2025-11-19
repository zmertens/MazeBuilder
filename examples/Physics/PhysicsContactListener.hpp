#pragma once

#ifndef PHYSICS_CONTACT_LISTENER_HPP
#define PHYSICS_CONTACT_LISTENER_HPP

#include <box2d/box2d.h>

// C-style Box2D contact callbacks
// These are registered with the world definition and forward contacts to Entity methods
bool PhysicsContactListener_BeginContact(b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Manifold* manifold, void* context);
void PhysicsContactListener_EndContact(b2ShapeId shapeIdA, b2ShapeId shapeIdB, void* context);
bool PhysicsContactListener_PreSolve(b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Manifold* manifold, void* context);

#endif // PHYSICS_CONTACT_LISTENER_HPP

