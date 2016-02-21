#include "KillDashNine.hpp"

#include "engine/Utils.hpp"

#if APP_DEBUG == 1
#include "engine/graphics/GlUtils.hpp"
#endif

// testing
#include "ResourcePaths.hpp"
#include "ResourceIds.hpp"
#include "ResourceLevels.hpp"
#include "engine/graphics/MaterialFactory.hpp"
#include "engine/graphics/Tex2dImpl.hpp"
#include "engine/graphics/TexSkyboxImpl.hpp"
#include "engine/graphics/TexPerlinNoise2dImpl.hpp"
#include "engine/graphics/MeshImpl.hpp"
#include "engine/graphics/IndexedMeshImpl.hpp"
#include "engine/graphics/MeshFactory.hpp"

const float KillDashNine::sTimePerFrame = 1.0f / 60.0f;
const glm::uvec2 KillDashNine::sWindowDimens = glm::uvec2(1080u, 720u);
const std::string KillDashNine::sTitle = "kill -9";

/**
 * @brief KillDashNine::KillDashNine
 */
KillDashNine::KillDashNine()
: mSdlManager(SdlWindow::Settings(SDL_INIT_VIDEO | SDL_INIT_AUDIO,
    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN, false),
    sWindowDimens, sTitle)
, mResources()
, mLogger()
, mAppIsRunning(false)
, mFrameCounter(0u)
, mTimeSinceLastUpdate(0.0f)
, mAccumulator(0.0f)
, mCube(Entity::Config(ResourceIds::Shaders::LEVEL_SHADER_ID,
    ResourceIds::Meshes::CUBE_ID,
    ResourceIds::Materials::PEARL_ID,
    ResourceIds::Textures::PERLIN_NOISE_2D_ID,
    Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::AWESOME_FACE_INDEX,
        ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS)),
    glm::vec3(2, 0, 0))
, mLevelGen(ResourceLevels::Levels::TEST_LEVEL,
    ResourceIds::Textures::Atlas::BRICKS2_INDEX, ResourceIds::Textures::Atlas::WALL_INDEX,
        ResourceIds::Textures::Atlas::METAL_INDEX,
    ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS,
    Entity::Config(ResourceIds::Shaders::LEVEL_SHADER_ID,
    ResourceIds::Meshes::LEVEL_ID,
    ResourceIds::Materials::PEARL_ID,
    ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID))
, mImGui(mSdlManager, mResources)
, mCamera(glm::vec3(0.0f), 0.0f, 0.0f, 75.0f, 0.1f, 1000.0f)
, mPlayer(mCamera, mLevelGen)
, mSkybox(Entity::Config(ResourceIds::Shaders::SKYBOX_SHADER_ID,
    ResourceIds::Meshes::VAO_ID,
    "",
    ResourceIds::Textures::SKYBOX_TEX_ID))
// @note this pp needs to load after the init function to prevent issues -- if using tex factory
, mPostProcessor(mResources, Entity::Config(ResourceIds::Shaders::EFFECTS_SHADER_ID,
    ResourceIds::Meshes::VAO_ID), sWindowDimens.x, sWindowDimens.y)
, mLight(glm::vec3(1), glm::vec3(1), glm::vec3(1), glm::vec4(0, 10.0f, 0, 0))
, mTestSprite(Entity::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
    ResourceIds::Meshes::VAO_ID,
    "",
    ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID,
    Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::AWESOME_FACE_INDEX,
        ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS)),
    glm::vec3(0.0f))
, mSdlMixer(mResources)
{
    // initialize all game resources (texture, material, mesh, et cetera)
    init();

    mPlayer.move(mLevelGen.getPlayerPosition(), 1.0f);

    for (auto& enemyPos : mLevelGen.getEnemyPositions())
    {
        mEnemies.emplace_back(std::move(new Enemy(
            mLevelGen.getTileScalar(),
            Entity::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
            ResourceIds::Meshes::VAO_ID,
            "",
            ResourceIds::Textures::Atlas::TEST_RPG_CHARS_ID,
            Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::RPG_1_WALK_1,
            ResourceIds::Textures::Atlas::TEST_RPG_CHARS_NUM_ROWS)),
            enemyPos
        )));
    }

    for (auto& pos : mLevelGen.getInvinciblePowerUps())
    {
        mPowerUps.emplace_back(std::move(new Sprite(
            Entity::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
            ResourceIds::Meshes::VAO_ID,
            "",
            ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID,
            Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::BREAKOUT_POWER_UP_CHAOS,
            ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS)),
            pos
        )));
    }

    for (auto& pos : mLevelGen.getSpeedPowerUps())
    {
        mPowerUps.emplace_back(std::move(new Sprite(
            Entity::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
            ResourceIds::Meshes::VAO_ID,
            "",
            ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID,
            Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::BREAKOUT_POWER_UP_CONFUSE,
            ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS)),
            pos
        )));
    }

    for (auto& pos : mLevelGen.getRechargePowerUps())
    {
        mPowerUps.emplace_back(std::move(new Sprite(
            Entity::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
            ResourceIds::Meshes::VAO_ID,
            "",
            ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID,
            Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::BREAKOUT_POWER_UP_INCREASE,
            ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS)),
            pos
        )));
    }
    // temp, get rid of this
    //mSdlMixer.playMusic(ResourceIds::Music::SOBER_LULLABY_MP3_ID, -1);
} // constructor

/**
 * @brief KillDashNine::start
 */
void KillDashNine::start()
{
    mAppIsRunning = true;
    loop();
}

/**
 * @brief KillDashNine::loop
 */
void KillDashNine::loop()
{
    while (mAppIsRunning)
    {
        static double lastTime = static_cast<double>(SDL_GetTicks()) / 1000.0;
        double currentTime = static_cast<double>(SDL_GetTicks()) / 1000.0;
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        mAccumulator += deltaTime;

        while (mAccumulator > sTimePerFrame)
        {
            mAccumulator -= sTimePerFrame;
            handleEvents();
            update(sTimePerFrame, currentTime);
        }

        render();

        printFramesToConsole(deltaTime);
    }

    finish();
}

/**
 * @brief KillDashNine::handleEvents
 */
void KillDashNine::handleEvents()
{
    float mouseWheelDy = 0;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        sdlEvents(event, mouseWheelDy);
    } // events

    // handle realtime input
    mPlayer.input(mSdlManager, mouseWheelDy);
} // update

/**
 * @brief KillDashNine::update
 * @param dt
 * @param timeSinceInit
 */
void KillDashNine::update(float dt, double timeSinceInit)
{
    //mCube.update(dt, timeSinceInit);
    mTestSprite.update(dt, timeSinceInit);

    //auto& transform = mTestSprite.getTransform();
    Transform transform (mLevelGen.getExitPoints().front(), glm::vec3(0), glm::vec3(1.1f));
    mTestSprite.setTransform(transform);

    mPlayer.update(dt, timeSinceInit);
    mLevelGen.update(dt, timeSinceInit);

    for (auto& enemy : mEnemies)
        enemy->update(dt, timeSinceInit);

    for (auto& powerup : mPowerUps)
        powerup->update(dt, timeSinceInit);

    mLight.setPosition(glm::vec4(mPlayer.getPosition().x, mLevelGen.getTileScalar().y - 2.0f, mPlayer.getPosition().z, 0.0f));

    static int testCounter = 0;
    if (++testCounter % 50 == 0)
    {
        //mSdlMixer.playChannel(-1, ResourceIds::Chunks::DEATH_WAV_ID, 2);
    }
}

/**
 * @brief KillDashNine::render
 */
void KillDashNine::render()
{
    mResources.clearCache();

    mPostProcessor.bind();

    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mSkybox.draw(mSdlManager, mResources, mCamera, IMesh::Draw::TRIANGLE_STRIP);

    auto& shader = mResources.getShader(ResourceIds::Shaders::LEVEL_SHADER_ID);
    shader->bind();
    auto& tex = mResources.getTexture(ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID);
    tex->bind();

    shader->setUniform("uLight.ambient", mLight.getAmbient());
    shader->setUniform("uLight.diffuse", mLight.getDiffuse());
    shader->setUniform("uLight.specular", mLight.getSpecular());
    shader->setUniform("uLight.position", mCamera.getLookAt() * mLight.getPosition());

    mLevelGen.draw(mSdlManager, mResources, mCamera);

    //mCube.draw(mSdlManager, mResources, mCamera, IMesh::Draw::TRIANGLES);

    mTestSprite.draw(mSdlManager, mResources, mCamera, IMesh::Draw::POINTS);

    auto& spriteShader = mResources.getShader(ResourceIds::Shaders::SPRITE_SHADER_ID);
    spriteShader->bind();
    spriteShader->setUniform("uHalfSize", mLevelGen.getSpriteHalfWidth());
    mResources.putInCache(ResourceIds::Shaders::SPRITE_SHADER_ID, CachePos::Shader);
    for (auto& enemy : mEnemies)
        enemy->draw(mSdlManager, mResources, mCamera, IMesh::Draw::POINTS);


    for (auto& powerup : mPowerUps)
        powerup->draw(mSdlManager, mResources, mCamera, IMesh::Draw::POINTS);

    mPostProcessor.activateEffect(Effects::Type::NO_EFFECT);
    mPostProcessor.release();

    mImGui.render();

    mSdlManager.swapBuffers();
}

/**
 *  @note The SdlManager must clean up before Resources
 *  or else GL errors will be thrown!
 *  @brief KillDashNine::finish
 */
void KillDashNine::finish()
{
    if (APP_DEBUG == 1)
    {
        mLogger.appendToLog(mSdlManager.getSdlInfoString());
        mLogger.appendToLog(mSdlManager.getGlInfoString());
        mLogger.appendToLog(mResources.getAllLogs());
        mLogger.dumpLogToFile("data_log.txt");
    }

    mAppIsRunning = false;
    mSdlManager.cleanUp();
    mResources.cleanUp();
}

/**
 * @brief KillDashNine::init
 */
void KillDashNine::init()
{
    //glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);

    // shaders
    Shader::Ptr level (new Shader(mSdlManager));
    level->compileAndAttachShader(ShaderTypes::VERTEX_SHADER,
        ResourcePaths::Shaders::LEVEL_VERTEX_SHADER_PATH);
    level->compileAndAttachShader(ShaderTypes::FRAGMENT_SHADER,
        ResourcePaths::Shaders::LEVEL_FRAGMENT_SHADER_PATH);
    level->linkProgram();
    level->bind();
    mResources.insert(ResourceIds::Shaders::LEVEL_SHADER_ID,
        std::move(level));

    Shader::Ptr skybox (new Shader(mSdlManager));
    skybox->compileAndAttachShader(ShaderTypes::VERTEX_SHADER,
        ResourcePaths::Shaders::SKYBOX_VERTEX_SHADER_PATH);
    skybox->compileAndAttachShader(ShaderTypes::FRAGMENT_SHADER,
        ResourcePaths::Shaders::SKYBOX_FRAGMENT_SHADER_PATH);
    skybox->linkProgram();
    skybox->bind();
    mResources.insert(ResourceIds::Shaders::SKYBOX_SHADER_ID,
        std::move(skybox));

    Shader::Ptr effects (new Shader(mSdlManager));
    effects->compileAndAttachShader(ShaderTypes::VERTEX_SHADER,
        ResourcePaths::Shaders::EFFECTS_VERTEX_SHADER_PATH);
    effects->compileAndAttachShader(ShaderTypes::FRAGMENT_SHADER,
        ResourcePaths::Shaders::EFFECTS_FRAGMENT_SHADER_PATH);
    effects->linkProgram();
    effects->bind();
    mResources.insert(ResourceIds::Shaders::EFFECTS_SHADER_ID,
        std::move(effects));

    Shader::Ptr spriteShader (new Shader(mSdlManager));
    spriteShader->compileAndAttachShader(ShaderTypes::VERTEX_SHADER,
        ResourcePaths::Shaders::SPRITE_VERTEX_SHADER_PATH);
    spriteShader->compileAndAttachShader(ShaderTypes::GEOMETRY_SHADER,
        ResourcePaths::Shaders::SPRITE_GEOM_SHADER_PATH);
    spriteShader->compileAndAttachShader(ShaderTypes::FRAGMENT_SHADER,
        ResourcePaths::Shaders::SPRITE_FRAGMENT_SHADER_PATH);
    spriteShader->linkProgram();
    spriteShader->bind();
    mResources.insert(ResourceIds::Shaders::SPRITE_SHADER_ID,
        std::move(spriteShader));

    // materials
    mResources.insert(ResourceIds::Materials::EMERALD_ID,
        MaterialFactory::ProduceMaterial(MaterialFactory::Types::EMERALD));
    mResources.insert(ResourceIds::Materials::OBSIDIAN_ID,
        MaterialFactory::ProduceMaterial(MaterialFactory::Types::OBSIDIAN));
    mResources.insert(ResourceIds::Materials::JADE_ID,
        MaterialFactory::ProduceMaterial(MaterialFactory::Types::JADE));
    mResources.insert(ResourceIds::Materials::PEARL_ID,
        MaterialFactory::ProduceMaterial(MaterialFactory::Types::PEARL));
    mResources.insert(ResourceIds::Materials::WHITE_ID,
        MaterialFactory::ProduceMaterial(MaterialFactory::Types::WHITE));
    mResources.insert(ResourceIds::Materials::CORAL_ORANGE_ID,
        MaterialFactory::ProduceMaterial(MaterialFactory::Types::CORAL_ORANGE));

    // meshes
    mResources.insert(ResourceIds::Meshes::CUBE_ID,
        std::move(MeshFactory::ProduceMesh(MeshFactory::Types::CUBE)));

    IMesh::Ptr vaoMesh (new MeshImpl());
    mResources.insert(ResourceIds::Meshes::VAO_ID, std::move(vaoMesh));

    std::vector<Vertex> vertices;
    std::vector<GLushort> indices;
    mLevelGen.generateLevel(vertices, indices);
    IMesh::Ptr levelMesh (new IndexedMeshImpl(vertices, indices));
    mResources.insert(ResourceIds::Meshes::LEVEL_ID, std::move(levelMesh));

    // textures
    ITexture::Ptr testTex (new Tex2dImpl(mSdlManager,
        ResourcePaths::Textures::TEST_TEX_ATLAS_PATH, 0));
    mResources.insert(ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID, std::move(testTex));

    ITexture::Ptr skyboxTex (new TexSkyboxImpl(mSdlManager,
        ResourcePaths::Textures::SKYBOX_PATHS, 0));
    mResources.insert(ResourceIds::Textures::SKYBOX_TEX_ID, std::move(skyboxTex));

    // @TODO use the fullscreen texture in the post processor (switch order of init)
    ITexture::Ptr fullScreenTex (new Tex2dImpl(
        mSdlManager.getDimensions().x,
        mSdlManager.getDimensions().y, 0));
    mResources.insert(ResourceIds::Textures::FULLSCREEN_TEX_ID, std::move(fullScreenTex));

    ITexture::Ptr charsTex (new Tex2dImpl(mSdlManager,
        ResourcePaths::Textures::TEST_RPG_CHARS_PATH, 0));
    mResources.insert(ResourceIds::Textures::Atlas::TEST_RPG_CHARS_ID, std::move(charsTex));

    ITexture::Ptr perlinTex (new TexPerlinNoise2dImpl(4.0f, 0.5f, 128, 128, true, 0));
    mResources.insert(ResourceIds::Textures::PERLIN_NOISE_2D_ID, std::move(perlinTex));

    // music
    Music::Ptr soberLullaby (new Music(ResourcePaths::Music::SOBER_LULLABY_MP3_PATH));
    mResources.insert(ResourceIds::Music::SOBER_LULLABY_MP3_ID, std::move(soberLullaby));

    // sound
    Chunk::Ptr deathSound (new Chunk(ResourcePaths::Chunks::DEATH_WAV_PATH));
    mResources.insert(ResourceIds::Chunks::DEATH_WAV_ID, std::move(deathSound));

    Chunk::Ptr exitSound (new Chunk(ResourcePaths::Chunks::EXIT_WAV_PATH));
    mResources.insert(ResourceIds::Chunks::EXIT_WAV_ID, std::move(exitSound));

    Chunk::Ptr hitHurtSound (new Chunk(ResourcePaths::Chunks::HIT_HURT_WAV_PATH));
    mResources.insert(ResourceIds::Chunks::HIT_HURT_WAV_ID, std::move(hitHurtSound));

    Chunk::Ptr genPowerUpSound (new Chunk(ResourcePaths::Chunks::GENERAL_POWERUP_WAV_PATH));
    mResources.insert(ResourceIds::Chunks::GENERAL_POWERUP_WAV_ID, std::move(genPowerUpSound));

    Chunk::Ptr laserSound (new Chunk(ResourcePaths::Chunks::LASER_WAV_PATH));
    mResources.insert(ResourceIds::Chunks::LASER_WAV_ID, std::move(laserSound));

    Chunk::Ptr pJumpSound (new Chunk(ResourcePaths::Chunks::PLAYER_JUMP_WAV_PATH));
    mResources.insert(ResourceIds::Chunks::PLAYER_JUMP_ID, std::move(pJumpSound));

    Chunk::Ptr selectSound (new Chunk(ResourcePaths::Chunks::SELECT_WAV_PATH));
    mResources.insert(ResourceIds::Chunks::SELECT_WAV_ID, std::move(selectSound));

}

/**
 * @brief KillDashNine::printFramesToConsole
 * @param dt
 */
void KillDashNine::printFramesToConsole(const float dt)
{
    ++mFrameCounter;
    mTimeSinceLastUpdate += dt;
    if (mTimeSinceLastUpdate >= 1.0f)
    {
        if (APP_DEBUG == 1)
        {
            SDL_Log("FPS: %u\n", mFrameCounter);
            SDL_Log("time (us) / frame: %f\n", mTimeSinceLastUpdate / mFrameCounter);
            mLogger.appendToLog("FPS: " + Utils::toString(mFrameCounter));
            mLogger.appendToLog("\n");
            mLogger.appendToLog("time (us) / frame: " + Utils::toString(mTimeSinceLastUpdate / mFrameCounter));
            mLogger.appendToLog("\n");
        }

        mFrameCounter = 0;
        mTimeSinceLastUpdate -= 1.0f;
    }
}

/**
 * @brief KillDashNine::sdlEvents
 * @param event
 * @param mouseWheelDy
 */
void KillDashNine::sdlEvents(SDL_Event& event, float& mouseWheelDy)
{
    if (event.type == SDL_QUIT)
    {
        mAppIsRunning = false;
    }
    else if (event.type == SDL_WINDOWEVENT)
    {
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            unsigned int newWidth = event.window.data1;
            unsigned int newHeight = event.window.data2;
            glViewport(0, 0, newWidth, newHeight);
            mSdlManager.setDimensions(glm::uvec2(newWidth, newHeight));
            if (APP_DEBUG)
            {
                std::string resizeDimens = "Resize Event -- Width: "
                    + Utils::toString(newWidth) + ", Height: "
                    + Utils::toString(newHeight) + "\n";
                SDL_Log(resizeDimens.c_str());
            }
        }
    }
    else if (event.type == SDL_MOUSEWHEEL)
        mouseWheelDy = event.wheel.y;
    else if (event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.sym == SDLK_RETURN)
            mSdlManager.toggleFullScreen();
        else if (event.key.keysym.sym == SDLK_ESCAPE)
            mAppIsRunning = false;
        else if (event.key.keysym.sym == SDLK_1)
        {

        }
    }
    else if ((mSdlManager.getWindowSettings().initFlags & SDL_INIT_JOYSTICK) &&
        event.type == SDL_JOYBUTTONDOWN)
    {
        if (event.jbutton.button == SDL_CONTROLLER_BUTTON_X &&
            mSdlManager.hapticRumblePlay(0.75, 500) != 0 && APP_DEBUG)
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, SDL_GetError());
    }
} // sdlEvents
