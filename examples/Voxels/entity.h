#ifndef ENTITY_H
#define ENTITY_H

enum class Entity : unsigned int
{
    NONE = 0,
    SCENE = 1 << 0,
    PLAYER = 1 << 1,
    ENEMY = 1 << 2,
    PROJECTILE = 1 << 3,
    PICKUP = 1 << 4,
    ALL = 1 << 5
};

#endif // ENTITY_H