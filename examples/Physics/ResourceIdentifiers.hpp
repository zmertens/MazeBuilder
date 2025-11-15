#ifndef RESOURCE_IDENTIFIERS_HPP
#define RESOURCE_IDENTIFIERS_HPP

#include <string_view>

namespace JSONKeys
{
    constexpr std::string_view CHARACTER_IMAGE = "character_default_image";
    constexpr std::string_view CHARACTERS_SPRITE_SHEET = "characters_spritesheet";
    constexpr std::string_view BALL_NORMAL = "ball_normal";
    constexpr std::string_view ENEMY_HITPOINTS_DEFAULT = "enemy_hitpoints_default";
    constexpr std::string_view ENEMY_SPEED_DEFAULT = "enemy_speed_default";
    constexpr std::string_view EXPLOSIONS_SPRITE_SHEET = "explosion_spritesheet";
    constexpr std::string_view LEVEL_DEFAULTS = "level_defaults";
    constexpr std::string_view NETWORK_URL = "network_url";
    constexpr std::string_view OGG_FILES = "ogg_files";
    constexpr std::string_view PLAYER_HITPOINTS_DEFAULT = "player_hitpoints_default";
    constexpr std::string_view PLAYER_SPEED_DEFAULT = "player_speed_default";
    constexpr std::string_view SDL_LOGO = "SDL_logo";
    constexpr std::string_view SFML_LOGO = "SFML_logo";
    constexpr std::string_view SPLASH_IMAGE = "splash_image";
    constexpr std::string_view WALL_HORIZONTAL = "wall_horizontal";
    constexpr std::string_view WAV_FILES = "wav_files";
    constexpr std::string_view WINDOW_ICON = "window_icon";
}

namespace Textures
{
    enum class ID : unsigned int
    {
        BALL_NORMAL = 0,
        CHARACTER = 1,
        CHARACTER_SPRITE_SHEET = 2,
        LEVEL_ONE = 3,
        LEVEL_TWO = 4,
        LEVEL_THREE = 5,
        LEVEL_FOUR = 6,
        LEVEL_FIVE = 7,
        LEVEL_SIX = 8,
        SDL_LOGO = 9,
        SFML_LOGO = 10,
        SPLASH_TITLE_IMAGE = 11,
        WALL_HORIZONTAL = 12,
        WINDOW_ICON = 13,
        TOTAL_IDS = 14
    };
}

namespace Fonts
{
    enum class ID : unsigned int
    {
        COUSINE_REGULAR = 0,
        LIMELIGHT = 1,
        NUNITO_SANS = 2
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
