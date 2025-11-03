#ifndef CATEGORY_HPP
#define CATEGORY_HPP


namespace Category
{
    enum class Type : unsigned int
    {
        NONE = 0,
        SCENE = 1 << 0,
        PLAYER = 1 << 1,
        ENEMY = 1 << 2,
        PROJECTILE = 1 << 3,
        PICKUP = 1 << 4,
        ALL = 1 << 5
    };
}

#endif // CATEGORY_HPP
