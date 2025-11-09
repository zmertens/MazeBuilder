#ifndef RESOURCE_IDENTIFIERS_HPP
#define RESOURCE_IDENTIFIERS_HPP

namespace Textures
{
    enum class ID
    {
        ASTRONAUT = 0,
        MAZE_BINARY_TREE = 1,
        MAZE_DFS = 2,
        MAZE_SIDEWINDER = 3,
        SPLASH_SCREEN = 4,
        BALL_NORMAL = 5,
        WALL_CORNER = 6,
        WALL_HORIZONTAL = 7,
        WALL_VERTICAL = 8,
        SDL_BLOCKS = 9
    };
}

namespace Fonts
{
    enum class ID
    {
        COUSINE_REGULAR,
        LIMELIGHT,
        NUNITO_SANS
    };
}

class Texture;
class Font;

// Forward declaration and a few type definitions
template <typename Resource, typename Identifier>
class ResourceManager;

typedef ResourceManager<Texture, Textures::ID> TextureManager;
typedef ResourceManager<Font, Fonts::ID> FontManager;

#endif // RESOURCE_IDENTIFIERS_HPP
