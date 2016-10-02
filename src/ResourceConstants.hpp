#ifndef RESOURCECONSTANTS
#define RESOURCECONSTANTS

#include <string>
#include <vector>

/**
 * All ID values are equal to the variable
 * name that sets it.
 */
namespace ResourceIds
{
namespace Shaders
{
static const std::string LEVEL_SHADER_ID = "LEVEL_SHADER_ID";
static const std::string SKYBOX_SHADER_ID = "SKYBOX_SHADER_ID";
static const std::string EFFECTS_SHADER_ID = "EFFECTS_SHADER_ID";
static const std::string SPRITE_SHADER_ID = "SPRITE_SHADER_ID";
static const std::string PARTICLES_SHADER_ID = "PARTICLES_SHADER_ID";
// const std::string TEXT_SHADER_ID = "TEXT_SHADER_ID";
}

namespace Textures
{
static const std::string SKYBOX_TEX_ID = "SKYBOX_TEX_ID";
static const std::string FULLSCREEN_TEX_ID = "FULLSCREEN_TEX_ID";
static const std::string PERLIN_NOISE_2D_ID = "PERLIN_NOISE_2D_ID";
static const std::string BLUEWATER_ID = "BLUEWATER_ID";

namespace Atlas
{
static constexpr unsigned int ATLAS_TEX_NUM_ROWS = 8u;
static const std::string LEVEL_ATLAS_TEX_ID = "LEVEL_ATLAS_TEX_ID";
static constexpr unsigned int MONTY_PYTHON_INDEX = 0u;
static constexpr unsigned int UP_ARROW_INDEX = 1u;
static constexpr unsigned int AWESOME_FACE_INDEX = 2u;
static constexpr unsigned int BRICKS2_INDEX = 3u;
static constexpr unsigned int BRICKS2_DISP_INDEX = 4u;
static constexpr unsigned int BRICKS2_NORMAL_INDEX = 5u;
static constexpr unsigned int BRICKWALL_INDEX = 6u;
static constexpr unsigned int BRICKWALL_NORMAL_INDEX = 7u;
static constexpr unsigned int CONTAINER_INDEX = 8u;
static constexpr unsigned int CONTAINER2_INDEX = 9u;
static constexpr unsigned int CONTAINER2_SPECULAR_INDEX = 10u;
static constexpr unsigned int GRASS_INDEX = 11u;
static constexpr unsigned int MARBLE_INDEX = 12u;
static constexpr unsigned int METAL_INDEX = 13u;
static constexpr unsigned int TOYBOX_DIFFUSE_INDEX = 14u;
static constexpr unsigned int TOYBOX_DISP_INDEX = 15u;
static constexpr unsigned int TOYBOX_NORMAL_INDEX = 16u;
static constexpr unsigned int WALL_INDEX = 17u;
static constexpr unsigned int WINDOW_INDEX = 18u;
static constexpr unsigned int WOOD_INDEX = 19u;
static constexpr unsigned int PLANET_INDEX = 20u;
static constexpr unsigned int ROCK_INDEX = 21u;
static constexpr unsigned int CYBORG_DIFFUSE_INDEX = 22u;
static constexpr unsigned int CYBORG_SPECULAR_INDEX = 23u;
static constexpr unsigned int CYBORG_NORMAL_INDEX = 24u;
static constexpr unsigned int NANOSUIT_ARM_DIFFUSE_INDEX = 25u;
static constexpr unsigned int NANOSUIT_ARM_NORMAL_INDEX = 26u;
static constexpr unsigned int NANOSUIT_ARM_AMBIENT_INDEX = 27u;
static constexpr unsigned int NANOSUIT_ARM_SPECULAR_INDEX = 28u;
static constexpr unsigned int NANOSUIT_BODY_DIFFUSE_INDEX = 29u;
static constexpr unsigned int NANOSUIT_BODY_NORMAL_INDEX = 30u;
static constexpr unsigned int NANOSUIT_BODY_AMBIENT_INDEX = 31u;
static constexpr unsigned int NANOSUIT_BODY_SPECULAR_INDEX = 32u;
static constexpr unsigned int NANOSUIT_LIGHT_NORMAL_INDEX = 33u;
static constexpr unsigned int NANOSUIT_LIGHT_AMBIENT_INDEX = 34u;
static constexpr unsigned int NANOSUIT_LIGHT_DIFFUSE_INDEX = 35u;
static constexpr unsigned int NANOSUIT_HAND_DIFFUSE_INDEX = 36u;
static constexpr unsigned int NANOSUIT_HAND_NORMAL_INDEX = 37u;
static constexpr unsigned int NANOSUIT_HAND_AMBIENT_INDEX = 38u;
static constexpr unsigned int NANOSUIT_HAND_SPECULAR_INDEX = 39u;
static constexpr unsigned int NANOSUIT_HELMET_DIFFUSE_INDEX = 40u;
static constexpr unsigned int NANOSUIT_HELMET_NORMAL_INDEX = 41u;
static constexpr unsigned int NANOSUIT_HELMET_AMBIENT_INDEX = 42u;
static constexpr unsigned int NANOSUIT_HELMET_SPECULAR_INDEX = 43u;
static constexpr unsigned int NANOSUIT_LEG_DIFFUSE_INDEX = 44u;
static constexpr unsigned int NANOSUIT_LEG_NORMAL_INDEX = 45u;
static constexpr unsigned int NANOSUIT_LEG_AMBIENT_INDEX = 46u;
static constexpr unsigned int NANOSUIT_LEG_SPECULAR_INDEX = 47u;
static constexpr unsigned int BREAKOUT_BACKGROUND = 48u;
static constexpr unsigned int BREAKOUT_BLOCK = 49u;
static constexpr unsigned int BREAKOUT_BLOCK_SOLID = 50u;
static constexpr unsigned int BREAKOUT_PADDLE = 51u;
static constexpr unsigned int BREAKOUT_PARTICLE = 52u;
static constexpr unsigned int BREAKOUT_POWER_UP_CHAOS = 53u;
static constexpr unsigned int BREAKOUT_POWER_UP_CONFUSE = 54u;
static constexpr unsigned int BREAKOUT_POWER_UP_INCREASE = 55u;
static constexpr unsigned int BREAKOUT_POWER_UP_PASSTHROUGH = 56u;
static constexpr unsigned int BREAKOUT_POWER_UP_SPEED = 57u;
static constexpr unsigned int BREAKOUT_POWER_UP_STICKY = 58u;

const std::string ENEMY_ATLAS_TEX_ID = "ENEMY_ATLAS_TEX_ID";
static constexpr unsigned int IDLE_0 = 0u;
static constexpr unsigned int IDLE_1 = 1u;
static constexpr unsigned int IDLE_2 = 2u;
static constexpr unsigned int ATTACK_0 = 3u;
static constexpr unsigned int ATTACK_1 = 4u;
static constexpr unsigned int ATTACK_2 = 5u;
static constexpr unsigned int HURT_0 = 6u;
static constexpr unsigned int HURT_1 = 7u;
static constexpr unsigned int HURT_2 = 8u;
static constexpr unsigned int DEAD_0 = 9u;
static constexpr unsigned int DEAD_1 = 10u;
static constexpr unsigned int DEAD_2 = 11u;
} // namespace Atlas
} // namespace Textures

namespace Meshes
{
const std::string VAO_ID = "VAO_ID";
const std::string SKYBOX_ID = "SKYBOX_ID";
const std::string FULLSCREEN_QUAD_ID = "FULLSCREEN_QUAD_ID";
const std::string LEVEL_ID = "LEVEL_ID";
const std::string PLANE_ID = "PLANE_ID";
const std::string CUBE_ID = "CUBE_ID";
const std::string TRIANGLE_ID = "TRIANGLE_ID";
const std::string MONKEY_ID = "MONKEY_ID";
const std::string SPHERE_ID = "SPHERE_ID";
const std::string ROCK_ID = "ROCK_ID";
}

namespace Materials
{
const std::string EMERALD_ID = "EMERALD_ID";
const std::string JADE_ID = "JADE_ID";
const std::string OBSIDIAN_ID = "OBSIDIAN_ID";
const std::string PEARL_ID = "PEARL_ID";
const std::string WHITE_ID = "WHITE_ID";
const std::string CORAL_ORANGE_ID = "CORAL_ORANGE_ID";
// @TODO add more materials
}

namespace Music
{
const std::string WRATH_OF_SIN_ID = "WRATH_OF_SIN_ID";
}

namespace Chunks
{
const std::string DEATH_WAV_ID = "DEATH_WAV_ID";
const std::string EXIT_WAV_ID = "EXIT_WAV_ID";
const std::string GENERAL_POWERUP_WAV_ID = "GENERAL_POWERUP_WAV_ID";
const std::string HIT_HURT_WAV_ID = "HIT_HURT_WAV_ID";
const std::string LASER_WAV_ID = "LASER_WAV_ID";
const std::string PLAYER_JUMP_ID = "PLAYER_JUMP_ID";
const std::string SELECT_WAV_ID = "SELECT_WAV_ID";
}

namespace Fonts
{
const std::string UBUNTU_FONT_ID = "UBUNTU_FONT_ID";
}

} // namespace

/**
 * SWL_RWops function will automatically look in assets directory
 * on Android operating system. In order to accomadate for this difference
 * on both Android and Desktop,
 * all the asset files are copied into the assets directory before building
 * the Android APK file. This is also the reason for the lack of the resources
 * directory prefix for the Android string paths.
 */
namespace ResourcePaths
{
namespace Shaders
{
const std::string LEVEL_VERTEX_SHADER_PATH = "./resources/shaders/level.vert.glsl";
const std::string LEVEL_FRAGMENT_SHADER_PATH = "./resources/shaders/level.frag.glsl";
const std::string SKYBOX_VERTEX_SHADER_PATH = "./resources/shaders/skybox.vert.glsl";
const std::string SKYBOX_FRAGMENT_SHADER_PATH = "./resources/shaders/skybox.frag.glsl";
const std::string EFFECTS_VERTEX_SHADER_PATH = "./resources/shaders/effects.vert.glsl";
const std::string EFFECTS_FRAGMENT_SHADER_PATH = "./resources/shaders/effects.frag.glsl";
const std::string SPRITE_VERTEX_SHADER_PATH = "./resources/shaders/sprite.vert.glsl";
const std::string SPRITE_GEOM_SHADER_PATH = "./resources/shaders/sprite.geom.glsl";
const std::string SPRITE_FRAGMENT_SHADER_PATH = "./resources/shaders/sprite.frag.glsl";
const std::string PARTICLES_VERTEX_SHADER_PATH = "./resources/shaders/particles.vert.glsl";
const std::string PARTICLES_FRAGMENT_SHADER_PATH = "./resources/shaders/particles.frag.glsl";
}

namespace Textures
{
const std::string LEVEL_ATLAS_TEX_PATH = "./resources/textures/level.png";
const std::string SKYBOX_TEX_TOP = "./resources/textures/skybox/top.png";
const std::string SKYBOX_TEX_BOTTOM = "./resources/textures/skybox/bottom.png";
const std::string SKYBOX_TEX_LEFT = "./resources/textures/skybox/left.png";
const std::string SKYBOX_TEX_RIGHT = "./resources/textures/skybox/right.png";
const std::string SKYBOX_TEX_BACK = "./resources/textures/skybox/back.png";
const std::string SKYBOX_TEX_FRONT = "./resources/textures/skybox/front.png";

const std::vector<std::string> SKYBOX_PATHS = {
    ResourcePaths::Textures::SKYBOX_TEX_RIGHT, // +x
    ResourcePaths::Textures::SKYBOX_TEX_LEFT,// -x
    ResourcePaths::Textures::SKYBOX_TEX_TOP, // +y
    ResourcePaths::Textures::SKYBOX_TEX_BOTTOM, // -y
    ResourcePaths::Textures::SKYBOX_TEX_BACK, // +z
    ResourcePaths::Textures::SKYBOX_TEX_FRONT, // -z
};

const std::string ENEMY_ATLAS_TEX_PATH = "./resources/textures/enemy.png";
const std::string BLUEWATER_PATH = "./resources/textures/bluewater.png";
}

namespace Music
{
const std::string WRATH_OF_SIN_MP3_PATH = "./resources/audio/Wrath_Of_Sin.mp3";
}

namespace Chunks
{
const std::string DEATH_WAV_PATH = "./resources/audio/death.wav";
const std::string EXIT_WAV_PATH = "./resources/audio/exit.wav";
const std::string GENERAL_POWERUP_WAV_PATH = "./resources/audio/general_powerup.wav";
const std::string HIT_HURT_WAV_PATH = "./resources/audio/hit-hurt.wav";
const std::string LASER_WAV_PATH = "./resources/audio/laser.wav";
const std::string PLAYER_JUMP_WAV_PATH = "./resources/audio/player_jump.wav";
const std::string SELECT_WAV_PATH = "./resources/audio/select.wav";
}

namespace Fonts
{
const std::string UBUNTU_FONT_PATH = "./resources/fonts/Ubuntu-M.ttf";
}

} // namespace

#endif // RESOURCECONSTANTS

