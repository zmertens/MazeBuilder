#ifndef RESOURCEPATHS
#define RESOURCEPATHS

#include <vector>
#include <string>

#include "engine/Config.hpp"

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
const std::string LEVEL_VERTEX_SHADER_PATH = "../resources/shaders/level.vert.glsl";
const std::string LEVEL_FRAGMENT_SHADER_PATH = "../resources/shaders/level.frag.glsl";
const std::string SKYBOX_VERTEX_SHADER_PATH = "../resources/shaders/skybox.vert.glsl";
const std::string SKYBOX_FRAGMENT_SHADER_PATH = "../resources/shaders/skybox.frag.glsl";
const std::string EFFECTS_VERTEX_SHADER_PATH = "../resources/shaders/effects.vert.glsl";
const std::string EFFECTS_FRAGMENT_SHADER_PATH = "../resources/shaders/effects.frag.glsl";
const std::string SPRITE_VERTEX_SHADER_PATH = "../resources/shaders/sprite.vert.glsl";
const std::string SPRITE_GEOM_SHADER_PATH = "../resources/shaders/sprite.geom.glsl";
const std::string SPRITE_FRAGMENT_SHADER_PATH = "../resources/shaders/sprite.frag.glsl";
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
const std::string TEST_TEX_ATLAS_PATH = "../resources/textures/test-atlas.png";
const std::string SKYBOX_TEX_TOP = "../resources/textures/skybox/top.png";
const std::string SKYBOX_TEX_BOTTOM = "../resources/textures/skybox/bottom.png";
const std::string SKYBOX_TEX_LEFT = "../resources/textures/skybox/left.png";
const std::string SKYBOX_TEX_RIGHT = "../resources/textures/skybox/right.png";
const std::string SKYBOX_TEX_BACK = "../resources/textures/skybox/back.png";
const std::string SKYBOX_TEX_FRONT = "../resources/textures/skybox/front.png";

const std::vector<std::string> SKYBOX_PATHS = {
    ResourcePaths::Textures::SKYBOX_TEX_RIGHT, // +x
    ResourcePaths::Textures::SKYBOX_TEX_LEFT,// -x
    ResourcePaths::Textures::SKYBOX_TEX_TOP, // +y
    ResourcePaths::Textures::SKYBOX_TEX_BOTTOM, // -y
    ResourcePaths::Textures::SKYBOX_TEX_BACK, // +z
    ResourcePaths::Textures::SKYBOX_TEX_FRONT, // -z
};

const std::string TEST_RPG_CHARS_PATH = "../resources/textures/Putt-Putt-Chars.png";

#elif defined(APP_ANDROID)

#endif // defined
}

namespace Meshes
{
#if defined(APP_DESKTOP)
const std::string MONKEY_PATH = "../resources/models/monkey/monkey.obj";
#elif defined(APP_ANDROID)
const std::string MONKEY_PATH = "./models/monkey/monkey.obj";
#endif // defined
}

namespace Music
{
#if defined(APP_DESKTOP)
const std::string SOBER_LULLABY_MP3_PATH = "../resources/audio/mathgrant_sober_lullaby.mp3";
#endif
}

namespace Chunks
{
#if defined(APP_DESKTOP)
const std::string DEATH_WAV_PATH = "../resources/audio/death.wav";
const std::string EXIT_WAV_PATH = "../resources/audio/exit.wav";
const std::string GENERAL_POWERUP_WAV_PATH = "../resources/audio/general_powerup.wav";
const std::string HIT_HURT_WAV_PATH = "../resources/audio/hit-hurt.wav";
const std::string LASER_WAV_PATH = "../resources/audio/laser.wav";
const std::string PLAYER_JUMP_WAV_PATH = "../resources/audio/player_jump.wav";
const std::string SELECT_WAV_PATH = "../resources/audio/select.wav";
#endif
}

} // namespace

#endif // RESOURCEPATHS

