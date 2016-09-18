#ifndef RESOURCECONSTANTS
#define RESOURCECONSTANTS

#include <string>

/**
 * All ID values are equal to the variable
 * name that sets it.
 */
namespace ResourceIds
{
namespace Shaders
{
const std::string LEVEL_SHADER_ID = "LEVEL_SHADER_ID";
const std::string SKYBOX_SHADER_ID = "SKYBOX_SHADER_ID";
const std::string EFFECTS_SHADER_ID = "EFFECTS_SHADER_ID";
const std::string SPRITE_SHADER_ID = "SPRITE_SHADER_ID";
// const std::string TEXT_SHADER_ID = "TEXT_SHADER_ID";
}

namespace Textures
{
const std::string SKYBOX_TEX_ID = "SKYBOX_TEX_ID";
const std::string FULLSCREEN_TEX_ID = "FULLSCREEN_TEX_ID";
const std::string PERLIN_NOISE_2D_ID = "PERLIN_NOISE_2D_ID";

namespace Atlas
{
// Test-Atlas data
const std::string TEST_ATLAS_TEX_ID = "TEST_ATLAS_TEX_ID";
const unsigned int TEST_ATLAS_TEX_NUM_ROWS = 8u;
const unsigned int MONTY_PYTHON_INDEX = 0u;
const unsigned int UP_ARROW_INDEX = 1u;
const unsigned int AWESOME_FACE_INDEX = 2u;
const unsigned int BRICKS2_INDEX = 3u;
const unsigned int BRICKS2_DISP_INDEX = 4u;
const unsigned int BRICKS2_NORMAL_INDEX = 5u;
const unsigned int BRICKWALL_INDEX = 6u;
const unsigned int BRICKWALL_NORMAL_INDEX = 7u;
const unsigned int CONTAINER_INDEX = 8u;
const unsigned int CONTAINER2_INDEX = 9u;
const unsigned int CONTAINER2_SPECULAR_INDEX = 10u;
const unsigned int GRASS_INDEX = 11u;
const unsigned int MARBLE_INDEX = 12u;
const unsigned int METAL_INDEX = 13u;
const unsigned int TOYBOX_DIFFUSE_INDEX = 14u;
const unsigned int TOYBOX_DISP_INDEX = 15u;
const unsigned int TOYBOX_NORMAL_INDEX = 16u;
const unsigned int WALL_INDEX = 17u;
const unsigned int WINDOW_INDEX = 18u;
const unsigned int WOOD_INDEX = 19u;
const unsigned int PLANET_INDEX = 20u;
const unsigned int ROCK_INDEX = 21u;
const unsigned int CYBORG_DIFFUSE_INDEX = 22u;
const unsigned int CYBORG_SPECULAR_INDEX = 23u;
const unsigned int CYBORG_NORMAL_INDEX = 24u;
const unsigned int NANOSUIT_ARM_DIFFUSE_INDEX = 25u;
const unsigned int NANOSUIT_ARM_NORMAL_INDEX = 26u;
const unsigned int NANOSUIT_ARM_AMBIENT_INDEX = 27u;
const unsigned int NANOSUIT_ARM_SPECULAR_INDEX = 28u;
const unsigned int NANOSUIT_BODY_DIFFUSE_INDEX = 29u;
const unsigned int NANOSUIT_BODY_NORMAL_INDEX = 30u;
const unsigned int NANOSUIT_BODY_AMBIENT_INDEX = 31u;
const unsigned int NANOSUIT_BODY_SPECULAR_INDEX = 32u;
const unsigned int NANOSUIT_LIGHT_NORMAL_INDEX = 33u;
const unsigned int NANOSUIT_LIGHT_AMBIENT_INDEX = 34u;
const unsigned int NANOSUIT_LIGHT_DIFFUSE_INDEX = 35u;
const unsigned int NANOSUIT_HAND_DIFFUSE_INDEX = 36u;
const unsigned int NANOSUIT_HAND_NORMAL_INDEX = 37u;
const unsigned int NANOSUIT_HAND_AMBIENT_INDEX = 38u;
const unsigned int NANOSUIT_HAND_SPECULAR_INDEX = 39u;
const unsigned int NANOSUIT_HELMET_DIFFUSE_INDEX = 40u;
const unsigned int NANOSUIT_HELMET_NORMAL_INDEX = 41u;
const unsigned int NANOSUIT_HELMET_AMBIENT_INDEX = 42u;
const unsigned int NANOSUIT_HELMET_SPECULAR_INDEX = 43u;
const unsigned int NANOSUIT_LEG_DIFFUSE_INDEX = 44u;
const unsigned int NANOSUIT_LEG_NORMAL_INDEX = 45u;
const unsigned int NANOSUIT_LEG_AMBIENT_INDEX = 46u;
const unsigned int NANOSUIT_LEG_SPECULAR_INDEX = 47u;
const unsigned int BREAKOUT_BACKGROUND = 48u;
const unsigned int BREAKOUT_BLOCK = 49u;
const unsigned int BREAKOUT_BLOCK_SOLID = 50u;
const unsigned int BREAKOUT_PADDLE = 51u;
const unsigned int BREAKOUT_PARTICLE = 52u;
const unsigned int BREAKOUT_POWER_UP_CHAOS = 53u;
const unsigned int BREAKOUT_POWER_UP_CONFUSE = 54u;
const unsigned int BREAKOUT_POWER_UP_INCREASE = 55u;
const unsigned int BREAKOUT_POWER_UP_PASSTHROUGH = 56u;
const unsigned int BREAKOUT_POWER_UP_SPEED = 57u;
const unsigned int BREAKOUT_POWER_UP_STICKY = 58u;

const std::string TEST_RPG_CHARS_ID = "TEST_RPG_CHARS_ID";
const unsigned int TEST_RPG_CHARS_NUM_ROWS = 8u;
const unsigned int RPG_1_WALK_1 = 0u;
const unsigned int RPG_1_WALK_2 = 1u;
const unsigned int RPG_1_WALK_3 = 2u;
const unsigned int RPG_1_WALK_4 = 3u;
const unsigned int RPG_1_BACK_1 = 8u;
const unsigned int RPG_1_BACK_2 = 9u;
const unsigned int RPG_1_BACK_3 = 10u;
const unsigned int RPG_1_BACK_4 = 11u;
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
#if defined(APP_DESKTOP)
const std::string LEVEL_VERTEX_SHADER_PATH = "./resources/shaders/level.vert.glsl";
const std::string LEVEL_FRAGMENT_SHADER_PATH = "./resources/shaders/level.frag.glsl";
const std::string SKYBOX_VERTEX_SHADER_PATH = "./resources/shaders/skybox.vert.glsl";
const std::string SKYBOX_FRAGMENT_SHADER_PATH = "./resources/shaders/skybox.frag.glsl";
const std::string EFFECTS_VERTEX_SHADER_PATH = "./resources/shaders/effects.vert.glsl";
const std::string EFFECTS_FRAGMENT_SHADER_PATH = "./resources/shaders/effects.frag.glsl";
const std::string SPRITE_VERTEX_SHADER_PATH = "./resources/shaders/sprite.vert.glsl";
const std::string SPRITE_GEOM_SHADER_PATH = "./resources/shaders/sprite.geom.glsl";
const std::string SPRITE_FRAGMENT_SHADER_PATH = "./resources/shaders/sprite.frag.glsl";
// const std::string TEXT_VERTEX_SHADER_PATH = "./resources/shaders/text.vert.glsl";
// const std::string TEXT_FRAGMENT_SHADER_PATH = "./resources/shaders/text.frag.glsl";
#elif defined(APP_ANDROID)
//const std::string SCENE_VERTEX_SHADER_PATH = "./shaders/sceneShader.vert.glsl";
//const std::string SCENE_FRAGMENT_SHADER_PATH = "./shaders/sceneShader.frag.glsl";
//const std::string SKYBOX_VERTEX_SHADER_PATH = "./shaders/skybox.vert.glsl";
//const std::string SKYBOX_FRAGMENT_SHADER_PATH = "./shaders/skybox.frag.glsl";
//const std::string EFFECTS_VERTEX_SHADER_PATH = "./shaders/effectsShader.vert.glsl";
//const std::string EFFECTS_FRAGMENT_SHADER_PATH = "./shaders/effectsShader.frag.glsl";
#endif // defined
}

namespace Textures
{
#if defined(APP_DESKTOP)
const std::string TEST_TEX_ATLAS_PATH = "./resources/textures/test-atlas.png";
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

const std::string TEST_RPG_CHARS_PATH = "./resources/textures/Putt-Putt-Chars.png";

#elif defined(APP_ANDROID)

#endif // defined
}

namespace Music
{
#if defined(APP_DESKTOP)
const std::string WRATH_OF_SIN_MP3_PATH = "./resources/audio/Wrath_Of_Sin.mp3";
#endif
}

namespace Chunks
{
#if defined(APP_DESKTOP)
const std::string DEATH_WAV_PATH = "./resources/audio/death.wav";
const std::string EXIT_WAV_PATH = "./resources/audio/exit.wav";
const std::string GENERAL_POWERUP_WAV_PATH = "./resources/audio/general_powerup.wav";
const std::string HIT_HURT_WAV_PATH = "./resources/audio/hit-hurt.wav";
const std::string LASER_WAV_PATH = "./resources/audio/laser.wav";
const std::string PLAYER_JUMP_WAV_PATH = "./resources/audio/player_jump.wav";
const std::string SELECT_WAV_PATH = "./resources/audio/select.wav";
#endif
}

namespace Fonts
{
const std::string UBUNTU_FONT_PATH = "./resources/fonts/Ubuntu-M.ttf";
}

} // namespace

#endif // RESOURCECONSTANTS

