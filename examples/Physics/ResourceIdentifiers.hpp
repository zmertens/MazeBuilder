#ifndef RESOURCE_IDENTIFIERS_HPP
#define RESOURCE_IDENTIFIERS_HPP

#include <string_view>

namespace JSONKeys
{
    constexpr std::string_view ASTRONAUT = "astronaut";
    constexpr std::string_view BALL_NORMAL = "ball_normal";
    constexpr std::string_view SDL_BLOCKS = "sdl_blocks";
    constexpr std::string_view WALL_HORIZONTAL = "wall_horizontal";
    constexpr std::string_view WINDOW_ICON = "window_icon";
}

namespace Textures
{
    enum class ID
    {
        ASTRONAUT = 0,
        MAZE_BINARY_TREE = 1,
        MAZE_DFS = 2,
        MAZE_SIDEWINDER = 3,
        SDL_BLOCKS = 4,
        SPLASH_SCREEN = 5,
        BALL_NORMAL = 6,
        WALL_CORNER = 7,
        WALL_HORIZONTAL = 8,
        WALL_VERTICAL = 9,
        WINDOW_ICON = 10
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
