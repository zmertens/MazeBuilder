#include "Shooter.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "engine/Utils.hpp"

#if defined(APP_DEBUG)
#include "engine/graphics/GlUtils.hpp"
#endif // defined

// testing
#include "ResourceConstants.hpp"
#include "engine/graphics/MaterialFactory.hpp"
#include "engine/graphics/Tex2dImpl.hpp"
#include "engine/graphics/TexSkyboxImpl.hpp"
#include "engine/graphics/TexPerlinImpl.hpp"
#include "engine/graphics/MeshImpl.hpp"
#include "engine/graphics/IndexedMeshImpl.hpp"
#include "engine/graphics/MeshFactory.hpp"
#include "engine/Text.hpp"

const float Shooter::sTimePerFrame = 1.0f / 60.0f;
const unsigned int Shooter::sWindowWidth = 1080u;
const unsigned int Shooter::sWindowHeight = 720u;
const std::string Shooter::sTitle = "Shooter";
std::unordered_map<uint8_t, bool> Shooter::sKeyInputs;

/**
 * @brief Shooter::Shooter
 */
Shooter::Shooter()
: mSdlWindow(SDL_INIT_VIDEO | SDL_INIT_AUDIO,
    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN, false,
    sTitle, sWindowWidth, sWindowHeight)
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
, mCamera(glm::vec3(0.0f), 0.0f, 0.0f, 75.0f, 0.1f, 1000.0f)
, mLevel(StartLevel::TEST_LEVEL,
    ResourceIds::Textures::Atlas::BRICKS2_INDEX, ResourceIds::Textures::Atlas::WALL_INDEX,
        ResourceIds::Textures::Atlas::METAL_INDEX,
    ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS,
    Entity::Config(ResourceIds::Shaders::LEVEL_SHADER_ID,
    ResourceIds::Meshes::LEVEL_ID,
    ResourceIds::Materials::PEARL_ID,
    ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID))
, mPlayer(mCamera, mLevel)
, mSkybox(Entity::Config(ResourceIds::Shaders::SKYBOX_SHADER_ID,
    ResourceIds::Meshes::VAO_ID,
    "",
    ResourceIds::Textures::SKYBOX_TEX_ID))
// @note this pp needs to load after the init function to prevent issues -- if using tex factory
, mPostProcessor(mResources, Entity::Config(ResourceIds::Shaders::EFFECTS_SHADER_ID,
    ResourceIds::Meshes::VAO_ID), sWindowWidth, sWindowHeight)
, mLight(glm::vec3(1), glm::vec3(1), glm::vec3(1), glm::vec4(0, 10.0f, 0, 0))
, mTestSprite(Entity::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
    ResourceIds::Meshes::VAO_ID,
    "",
    ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID,
    Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::AWESOME_FACE_INDEX,
        ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS)),
    glm::vec3(0.0f))
, mRenderText()
, mSdlMixer(mResources)
{
    // initialize all game resources (texture, material, mesh, et cetera)
    init();
} // constructor

/**
 * @brief Shooter::start
 */
void Shooter::start()
{
    mAppIsRunning = true;

    mSdlMixer.playMusic(ResourceIds::Music::WRATH_OF_SIN_ID, -1);

    loop();
}

/**
 * @brief Shooter::loop
 */
void Shooter::loop()
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

        calcFrameRate(deltaTime);
    }

    finish();
}

/**
 * @brief Shooter::handleEvents
 */
void Shooter::handleEvents()
{
    float mouseWheelDy = 0;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        sdlEvents(event, mouseWheelDy);
    } // events

    const Uint8* currentKeyStates = SDL_GetKeyboardState(nullptr);
    SDL_PumpEvents(); // don't do this on a seperate thread?

    //sKeyInputs[SDL_SCANCODE_TAB] = static_cast<bool>(currentKeyStates[SDL_SCANCODE_TAB]);
    sKeyInputs[SDL_SCANCODE_W] = static_cast<bool>(currentKeyStates[SDL_SCANCODE_W]);
    sKeyInputs[SDL_SCANCODE_S] = static_cast<bool>(currentKeyStates[SDL_SCANCODE_S]);
    sKeyInputs[SDL_SCANCODE_A] = static_cast<bool>(currentKeyStates[SDL_SCANCODE_A]);
    sKeyInputs[SDL_SCANCODE_D] = static_cast<bool>(currentKeyStates[SDL_SCANCODE_D]);

    int coordX;
    int coordY;
    const Uint32 currentMouseStates = SDL_GetMouseState(&coordX, &coordY);
    glm::vec2 coords = glm::vec2(coordX, coordY);

    // handle realtime input
    mPlayer.input(mSdlWindow, mouseWheelDy, coords, sKeyInputs);
} // update

/**
 * @brief Shooter::update
 * @param dt = the time between frames
 * @param timeSinceInit = the time since SDL was initialized
 */
void Shooter::update(float dt, double timeSinceInit)
{
    //mCube.update(dt, timeSinceInit);
    mTestSprite.update(dt, timeSinceInit);

    //auto& transform = mTestSprite.getTransform();
    Transform transform (mLevel.getExitPoints().front(), glm::vec3(0), glm::vec3(1.1f));
    mTestSprite.setTransform(transform);

    mPlayer.update(dt, timeSinceInit);
    mLevel.update(dt, timeSinceInit);

    for (auto& enemy : mEnemies)
        enemy->update(dt, timeSinceInit);

    for (auto& powerup : mPowerUps)
        powerup->update(dt, timeSinceInit);

    // keep the light above the player's head
    mLight.setPosition(glm::vec4(mPlayer.getPosition().x,
        mLevel.getTileScalar().y - mPlayer.getPlayerSize().y,
        mPlayer.getPosition().z, 0.0f));

//    static int testCounter = 0;
//    if (++testCounter % 50 == 0)
//    {
//        //mSdlMixer.playChannel(-1, ResourceIds::Chunks::DEATH_WAV_ID, 2);
//    }
}

/**
 * @brief Shooter::render
 */
void Shooter::render()
{
    mResources.clearCache();

    mPostProcessor.bind();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mSkybox.draw(mSdlWindow, mResources, mCamera, IMesh::Draw::TRIANGLE_STRIP);

    auto& shader = mResources.getShader(ResourceIds::Shaders::LEVEL_SHADER_ID);
    shader->bind();
    auto& tex = mResources.getTexture(ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID);
    tex->bind();

    shader->setUniform("uLight.ambient", mLight.getAmbient());
    shader->setUniform("uLight.diffuse", mLight.getDiffuse());
    shader->setUniform("uLight.specular", mLight.getSpecular());
    shader->setUniform("uLight.position", mCamera.getLookAt() * mLight.getPosition());

    mLevel.draw(mSdlWindow, mResources, mCamera);

    //mCube.draw(mSdlWindow, mResources, mCamera, IMesh::Draw::TRIANGLES);

    mTestSprite.draw(mSdlWindow, mResources, mCamera, IMesh::Draw::POINTS);

    auto& spriteShader = mResources.getShader(ResourceIds::Shaders::SPRITE_SHADER_ID);
    spriteShader->bind();
    spriteShader->setUniform("uHalfSize", mLevel.getSpriteHalfWidth());
    mResources.putInCache(ResourceIds::Shaders::SPRITE_SHADER_ID, CachePos::Shader);
    for (auto& enemy : mEnemies)
        enemy->draw(mSdlWindow, mResources, mCamera, IMesh::Draw::POINTS);


    for (auto& powerup : mPowerUps)
        powerup->draw(mSdlWindow, mResources, mCamera, IMesh::Draw::POINTS);

    mPostProcessor.activateEffect(Effects::Type::NO_EFFECT);
    mPostProcessor.release();


    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    // auto& txtShader = mResources.getShader(ResourceIds::Shaders::TEXT_SHADER_ID);
    // txtShader->bind();

    // auto& font = mResources.getFont(ResourceIds::Fonts::UBUNTU_FONT_ID);
    // font->bindTexture();

    // txtShader->setUniform("uProjection", glm::ortho(0.0f, static_cast<float>(mSdlWindow.getWindowWidth()), 0.0f, static_cast<float>(mSdlWindow.getWindowHeight())));
    // txtShader->setUniform("uColor", glm::vec3(1));
    // BoundingBox box {glm::vec3(50, 50, 0), glm::vec3(0)};
    // const Text sampleText {"SAMPLEasdasdasd", box, ResourceIds::Fonts::UBUNTU_FONT_ID};
    // mRenderText.renderText(mResources, sampleText);
    // glDisable(GL_BLEND);

    mSdlWindow.swapBuffers();
}

/**
 *  @note The SdlManager must clean up before Resources
 *  or else GL errors will be thrown!
 *  @brief Shooter::finish
 */
void Shooter::finish()
{
#if defined(APP_DEBUG)
    mLogger.appendToLog(mSdlWindow.getSdlInfoString());
    mLogger.appendToLog(mSdlWindow.getGlInfoString());
    mLogger.appendToLog(mResources.getAllLogs());
    mLogger.dumpLogToFile("data_log.txt");
#endif // defined

    mAppIsRunning = false;
    mSdlWindow.cleanUp();
    mResources.cleanUp();
}

/**
 * @brief Shooter::init
 */
void Shooter::init()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    initResources();
    initPositions();
}

/**
 * @brief Shooter::initResources
 */
void Shooter::initResources()
{
    // shaders
    Shader::Ptr level (new Shader(mSdlWindow));
    level->compileAndAttachShader(ShaderTypes::VERTEX_SHADER,
        ResourcePaths::Shaders::LEVEL_VERTEX_SHADER_PATH);
    level->compileAndAttachShader(ShaderTypes::FRAGMENT_SHADER,
        ResourcePaths::Shaders::LEVEL_FRAGMENT_SHADER_PATH);
    level->linkProgram();
    level->bind();
    mResources.insert(ResourceIds::Shaders::LEVEL_SHADER_ID,
        std::move(level));

    Shader::Ptr skybox (new Shader(mSdlWindow));
    skybox->compileAndAttachShader(ShaderTypes::VERTEX_SHADER,
        ResourcePaths::Shaders::SKYBOX_VERTEX_SHADER_PATH);
    skybox->compileAndAttachShader(ShaderTypes::FRAGMENT_SHADER,
        ResourcePaths::Shaders::SKYBOX_FRAGMENT_SHADER_PATH);
    skybox->linkProgram();
    skybox->bind();
    mResources.insert(ResourceIds::Shaders::SKYBOX_SHADER_ID,
        std::move(skybox));

    Shader::Ptr effects (new Shader(mSdlWindow));
    effects->compileAndAttachShader(ShaderTypes::VERTEX_SHADER,
        ResourcePaths::Shaders::EFFECTS_VERTEX_SHADER_PATH);
    effects->compileAndAttachShader(ShaderTypes::FRAGMENT_SHADER,
        ResourcePaths::Shaders::EFFECTS_FRAGMENT_SHADER_PATH);
    effects->linkProgram();
    effects->bind();
    mResources.insert(ResourceIds::Shaders::EFFECTS_SHADER_ID,
        std::move(effects));

    Shader::Ptr spriteShader (new Shader(mSdlWindow));
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

    Shader::Ptr txt (new Shader(mSdlWindow));
    txt->compileAndAttachShader(ShaderTypes::VERTEX_SHADER,
        ResourcePaths::Shaders::TEXT_VERTEX_SHADER_PATH);
    txt->compileAndAttachShader(ShaderTypes::FRAGMENT_SHADER,
        ResourcePaths::Shaders::TEXT_FRAGMENT_SHADER_PATH);
    txt->linkProgram();
    txt->bind();
    txt->setUniform("uTexture2D", 2);

    mResources.insert(ResourceIds::Shaders::TEXT_SHADER_ID,
        std::move(txt));

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
    mLevel.generateLevel(vertices, indices);
    IMesh::Ptr levelMesh (new IndexedMeshImpl(vertices, indices));
    mResources.insert(ResourceIds::Meshes::LEVEL_ID, std::move(levelMesh));

    // textures
    ITexture::Ptr testTex (new Tex2dImpl(mSdlWindow,
        ResourcePaths::Textures::TEST_TEX_ATLAS_PATH, 0));
    mResources.insert(ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID, std::move(testTex));

    ITexture::Ptr skyboxTex (new TexSkyboxImpl(mSdlWindow,
        ResourcePaths::Textures::SKYBOX_PATHS, 0));
    mResources.insert(ResourceIds::Textures::SKYBOX_TEX_ID, std::move(skyboxTex));

    // @TODO use the fullscreen texture in the post processor (switch order of init) -- add FBO to RM
    ITexture::Ptr fullScreenTex (new Tex2dImpl(
        mSdlWindow.getWindowWidth(), mSdlWindow.getWindowHeight(), 0));
    mResources.insert(ResourceIds::Textures::FULLSCREEN_TEX_ID, std::move(fullScreenTex));

    ITexture::Ptr charsTex (new Tex2dImpl(mSdlWindow,
        ResourcePaths::Textures::TEST_RPG_CHARS_PATH, 0));
    mResources.insert(ResourceIds::Textures::Atlas::TEST_RPG_CHARS_ID, std::move(charsTex));

    ITexture::Ptr perlinTex (new TexPerlinNoise2dImpl(4.0f, 0.5f, 128, 128, true, 0));
    mResources.insert(ResourceIds::Textures::PERLIN_NOISE_2D_ID, std::move(perlinTex));

    // music
    Music::Ptr mus (new Music(ResourcePaths::Music::WRATH_OF_SIN_MP3_PATH));
    mResources.insert(ResourceIds::Music::WRATH_OF_SIN_ID, std::move(mus));

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

    // fonts
    // Font::Ptr ubuntuFont (new Font(mSdlWindow, ResourcePaths::Fonts::UBUNTU_FONT_PATH, 48l));
    // mResources.insert(ResourceIds::Fonts::UBUNTU_FONT_ID, std::move(ubuntuFont));
}

/**
 * @brief Shooter::initPositions
 */
void Shooter::initPositions()
{
    mPlayer.move(mLevel.getPlayerPosition(), 1.0f);

    for (auto& enemyPos : mLevel.getEnemyPositions())
    {
        mEnemies.emplace_back(std::move(new Enemy(
            mLevel.getTileScalar(),
            Entity::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
            ResourceIds::Meshes::VAO_ID,
            "",
            ResourceIds::Textures::Atlas::TEST_RPG_CHARS_ID,
            Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::RPG_1_WALK_1,
            ResourceIds::Textures::Atlas::TEST_RPG_CHARS_NUM_ROWS)),
            enemyPos
        )));
    }

    for (auto& pos : mLevel.getInvinciblePowerUps())
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

    for (auto& pos : mLevel.getSpeedPowerUps())
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

    for (auto& pos : mLevel.getRechargePowerUps())
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
}

/**
 * @brief Shooter::calcFrameRate
 * @param dt
 */
void Shooter::calcFrameRate(const float dt)
{
    ++mFrameCounter;
    mTimeSinceLastUpdate += dt;
    if (mTimeSinceLastUpdate >= 1.0f)
    {
#if defined(APP_DEBUG)
        SDL_Log("FPS: %u\n", mFrameCounter);
        SDL_Log("time (us) / frame: %f\n", mTimeSinceLastUpdate / mFrameCounter);
        mLogger.appendToLog("FPS: " + Utils::toString(mFrameCounter));
        mLogger.appendToLog("\n");
        mLogger.appendToLog("time (us) / frame: " + Utils::toString(mTimeSinceLastUpdate / mFrameCounter));
        mLogger.appendToLog("\n");
#endif // defined
        mFrameCounter = 0;
        mTimeSinceLastUpdate -= 1.0f;
    }
}

/**
 * @brief Shooter::sdlEvents
 * @param event
 * @param mouseWheelDy
 */
void Shooter::sdlEvents(SDL_Event& event, float& mouseWheelDy)
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

            mSdlWindow.setWindowWidth(newWidth);
            mSdlWindow.setWindowHeight(newHeight);

#if defined(APP_DEBUG)
            std::string resizeDimens = "Resize Event -- Width: "
                + Utils::toString(newWidth) + ", Height: "
                + Utils::toString(newHeight) + "\n";
            SDL_Log(resizeDimens.c_str());
#endif // defined
        }
    }
    else if (event.type == SDL_MOUSEWHEEL)
        mouseWheelDy = event.wheel.y;
    else if (event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.sym == SDLK_TAB)
        {
            // flip mouse lock
            mPlayer.setMouseLocked(!mPlayer.getMouseLocked());
            if (mPlayer.getMouseLocked())
                SDL_ShowCursor(SDL_DISABLE);
            else
                SDL_ShowCursor(SDL_ENABLE);
        }
        else if (event.key.keysym.sym == SDLK_ESCAPE)
            mAppIsRunning = false;
    }
    else if ((mSdlWindow.getInitFlags() & SDL_INIT_JOYSTICK) &&
        event.type == SDL_JOYBUTTONDOWN)
    {
#if defined(APP_DEBUG)
        if (event.jbutton.button == SDL_CONTROLLER_BUTTON_X &&
            mSdlWindow.hapticRumblePlay(0.75, 500) != 0)
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, SDL_GetError());
#endif // defined
    }
} // sdlEvents
