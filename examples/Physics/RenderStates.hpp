#ifndef RENDER_STATES_HPP
#define RENDER_STATES_HPP

#include <box2d/math_functions.h>

struct RenderStates
{
    RenderStates() : transform(b2Transform_identity)
    {
    }

    explicit RenderStates(const b2Transform& theTransform) : transform(theTransform)
    {
    }

    b2Transform transform;
};

#endif // RENDER_STATES_HPP
