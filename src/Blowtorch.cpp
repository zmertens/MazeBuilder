#include "Blowtorch.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include "engine/Utils.hpp"

#if defined(BLOWTORCH_DEBUG_MODE)
#include "engine/graphics/GlUtils.hpp"
#endif // defined

#include "ResourceConstants.hpp"
#include "engine/graphics/MaterialFactory.hpp"
#include "engine/graphics/Tex2dImpl.hpp"
#include "engine/graphics/TexSkyboxImpl.hpp"
#include "engine/graphics/TexPerlinImpl.hpp"
#include "engine/graphics/MeshImpl.hpp"
#include "engine/graphics/IndexedMeshImpl.hpp"
#include "engine/graphics/MeshFactory.hpp"

std::unordered_map<uint8_t, bool> Blowtorch::sKeyInputs;

/**
 * @brief Blowtorch::Blowtorch
 */
Blowtorch::Blowtorch()
: mSdlWindow(sTitle, sWindowWidth, sWindowHeight)
, mResources()
, mLogger()
, mPlay(false)
, mFrameCounter(0u)
, mTimeSinceLastUpdate(0.0f)
, mAccumulator(0.0f)

, mImGui(mSdlWindow, mResources)
, mSdlMixer(mResources)

/********* position,       yaw,  pitch, fov,  near, far  ******/
, mCamera(glm::vec3(0.0f), 0.0f, 0.0f, 75.0f, 0.1f, 1000.0f)
, mLevel(
    ResourceIds::Textures::Atlas::BRICKS2_INDEX, 
    ResourceIds::Textures::Atlas::WALL_INDEX,
    ResourceIds::Textures::Atlas::METAL_INDEX,
    ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS,
    
    Draw::Config(ResourceIds::Shaders::LEVEL_SHADER_ID, ResourceIds::Meshes::LEVEL_ID, ResourceIds::Materials::PEARL_ID, ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID))
, mPlayer(mCamera, mLevel)


// @TODO -- Initialize game entities after resource initialization (incudling post-processor)
, mCube(Draw::Config(ResourceIds::Shaders::LEVEL_SHADER_ID,
    ResourceIds::Meshes::CUBE_ID,
    ResourceIds::Materials::PEARL_ID,
    ResourceIds::Textures::PERLIN_NOISE_2D_ID,
    Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::AWESOME_FACE_INDEX, ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS)))
, mSkybox(Draw::Config(ResourceIds::Shaders::SKYBOX_SHADER_ID,
    ResourceIds::Meshes::VAO_ID,
    "",
    ResourceIds::Textures::SKYBOX_TEX_ID))
, mPostProcessor(mResources, Draw::Config(ResourceIds::Shaders::EFFECTS_SHADER_ID,
    ResourceIds::Meshes::VAO_ID), mSdlWindow.getWindowWidth(), mSdlWindow.getWindowHeight())
, mLight(glm::vec3(1), glm::vec3(1), glm::vec3(1), glm::vec4(0, 10.0f, 0, 0))
, mExitSprite(Draw::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
    ResourceIds::Meshes::VAO_ID,
    "",
    ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID,
    Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::AWESOME_FACE_INDEX, ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS)),
    glm::vec3(0.0f))
{
    init();

    auto&& t = mCube.getTransform();
    t.setTranslation(mPlayer.getPosition());
    mCube.setTransform(t);
} // constructor

/**
 * @brief Blowtorch::start
 */
void Blowtorch::start()
{
    mPlay = true;
    // mSdlMixer.playMusic(ResourceIds::Music::WRATH_OF_SIN_ID, -1);
    loop();
}

/**
 * @brief Blowtorch::loop
 */
void Blowtorch::loop()
{
    while (mPlay)
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
#if defined(BLOWTORCH_DEBUG_MODE)
    calcFrameRate(deltaTime);
#endif // defined
    }

    finish();
}

/**
 * @brief Blowtorch::handleEvents
 */
void Blowtorch::handleEvents()
{
    float mouseWheelDy = 0;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        mImGui.ImGui_ImplSdlGL3_ProcessEvent(&event);
        sdlEvents(event, mouseWheelDy);
    } // events

    SDL_PumpEvents(); // don't do this on a seperate thread they said
    const Uint8* currentKeyStates = SDL_GetKeyboardState(nullptr);

    sKeyInputs[SDL_SCANCODE_TAB] = static_cast<bool>(currentKeyStates[SDL_SCANCODE_TAB]);
    sKeyInputs[SDL_SCANCODE_W] = static_cast<bool>(currentKeyStates[SDL_SCANCODE_W]);
    sKeyInputs[SDL_SCANCODE_S] = static_cast<bool>(currentKeyStates[SDL_SCANCODE_S]);
    sKeyInputs[SDL_SCANCODE_A] = static_cast<bool>(currentKeyStates[SDL_SCANCODE_A]);
    sKeyInputs[SDL_SCANCODE_D] = static_cast<bool>(currentKeyStates[SDL_SCANCODE_D]);

    int coordX;
    int coordY;
    const Uint32 currentMouseStates = SDL_GetMouseState(&coordX, &coordY);

    // handle realtime input
    mPlayer.input(mSdlWindow, mouseWheelDy, static_cast<int32_t>(currentMouseStates), glm::vec2(coordX, coordY), sKeyInputs);
} // handleEvents

/**
 * @brief Blowtorch::update
 * @param dt = the time between frames
 * @param timeSinceInit = the time since SDL was initialized
 */
void Blowtorch::update(float dt, double timeSinceInit)
{
    mCube.update(dt, timeSinceInit);
    mExitSprite.update(dt, timeSinceInit);

    Transform transform (mLevel.getExitPoints().front(), glm::vec3(0), glm::vec3(0.9f));
    mExitSprite.setTransform(transform);

    mPlayer.update(dt, timeSinceInit);
    mLevel.update(dt, timeSinceInit);

    for (auto& enemy : mEnemies)
    {
        // dead bodies don't animate
        if (enemy->getState() == Enemy::States::Dead)
            continue;
        enemy->update(dt, timeSinceInit);
        enemy->handleMovement(dt, mPlayer, mLevel);
    }

    for (auto& powerup : mPowerUps)
        powerup->update(dt, timeSinceInit);

    // keep the light above the player's head
    mLight.setPosition(glm::vec4(mPlayer.getPosition().x, mLevel.getTileScalar().y - mPlayer.getPlayerSize(), mPlayer.getPosition().z, 0.0f));

    mParticles->update(dt, timeSinceInit);

    mImGui.update(*this);

//    static int testCounter = 0;
//    if (++testCounter % 50 == 0)
//    {
//        //mSdlMixer.playChannel(-1, ResourceIds::Chunks::DEATH_WAV_ID, 2);
//    }

    if (mPlayer.isOnExit())
    {
        mPlayer.move(glm::vec3(0), 1.0f);
        mPlayer.move(mLevel.getPlayerPosition(), 1.0f);
    }
}

/**
 * @brief Blowtorch::render
 */
void Blowtorch::render()
{
    mPostProcessor.bind();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mSkybox.draw(mSdlWindow, mResources, mCamera, IMesh::Draw::TRIANGLE_STRIP);

    auto& shader = mResources.getShader(ResourceIds::Shaders::LEVEL_SHADER_ID);
    shader->bind();
    // auto& tex = mResources.getTexture(ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID);
    // tex->bind();
    shader->setUniform("uLight.ambient", mLight.getAmbient());
    shader->setUniform("uLight.diffuse", mLight.getDiffuse());
    shader->setUniform("uLight.specular", mLight.getSpecular());
    shader->setUniform("uLight.position", mCamera.getLookAt() * mLight.getPosition());

    mLevel.draw(mSdlWindow, mResources, mCamera);
    mCube.draw(mSdlWindow, mResources, mCamera, IMesh::Draw::TRIANGLES);

    mParticles->draw(mSdlWindow, mResources, mCamera);

    auto& spriteShader = mResources.getShader(ResourceIds::Shaders::SPRITE_SHADER_ID);
    spriteShader->bind();
    spriteShader->setUniform("uHalfSize", mLevel.getSpriteHalfWidth());

    mExitSprite.draw(mSdlWindow, mResources, mCamera, IMesh::Draw::POINTS);

    for (auto& enemy : mEnemies)
        enemy->draw(mSdlWindow, mResources, mCamera, IMesh::Draw::POINTS);

    for (auto& powerup : mPowerUps)
        powerup->draw(mSdlWindow, mResources, mCamera, IMesh::Draw::POINTS);

    if (mPlayer.getPower() == Power::Type::Immunity)
        mPostProcessor.activateEffect(Effects::Type::Blur);
    else if (mPlayer.getPower() == Power::Type::Speed) 
        mPostProcessor.activateEffect(Effects::Type::Edge);
    else if (mPlayer.getPower() == Power::Type::Strength)
        mPostProcessor.activateEffect(Effects::Type::Inversion);
    else
        mPostProcessor.activateEffect(Effects::Type::None);
    
    mPostProcessor.release();

    mImGui.render();

    mSdlWindow.swapBuffers();
}

/**
 *  @brief Blowtorch::finish
 */
void Blowtorch::finish()
{
    mPlay = false;
#if defined(BLOWTORCH_DEBUG_MODE)
    mLogger.appendToLog(mSdlWindow.getSdlInfoString());
    mLogger.appendToLog(mSdlWindow.getGlInfoString());
    mLogger.appendToLog(mResources.getAllLogs());
    mLogger.dumpLogToFile(Utils::toString(sTitle) + "DataLog.txt");
#endif // defined

    mParticles->cleanUp();
    mResources.cleanUp();
    mImGui.cleanUp();
    mSdlWindow.cleanUp(); // call this guy last
}

/**
 * @brief Blowtorch::init
 */
void Blowtorch::init()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glPointSize(10.0f);

    initResources();
    initPositions();
}

/**
 * @brief Blowtorch::initResources
 */
void Blowtorch::initResources()
{
    /***************** Shaders ****************************/
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

    Shader::Ptr particles (new Shader(mSdlWindow));
    particles->compileAndAttachShader(ShaderTypes::VERTEX_SHADER,
        ResourcePaths::Shaders::PARTICLES_VERTEX_SHADER_PATH);
    particles->compileAndAttachShader(ShaderTypes::FRAGMENT_SHADER,
        ResourcePaths::Shaders::PARTICLES_FRAGMENT_SHADER_PATH);
    // setup xform feedback before linkage
    const char* names[] = {"Position", "Velocity", "StartTime"};
    particles->initTransformFeedback(3, names, GL_SEPARATE_ATTRIBS);
    particles->linkProgram();
    particles->bind();
    particles->setUniform("uRender", GL_FALSE);
    particles->setUniform("uParticleTex", 0);
    particles->setUniform("ParticleLifetime", 0.5f);
    particles->setUniform("Accel", glm::vec3(0.0f, 0.0f, -1.4f));
    mResources.insert(ResourceIds::Shaders::PARTICLES_SHADER_ID,
        std::move(particles));

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

    /***************** Materials **************************************/
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

    /************ Meshes ************************************************/
    mResources.insert(ResourceIds::Meshes::CUBE_ID,
        std::move(MeshFactory::ProduceMesh(MeshFactory::Types::CUBE)));

    IMesh::Ptr vaoMesh (new MeshImpl());
    mResources.insert(ResourceIds::Meshes::VAO_ID, std::move(vaoMesh));

    std::vector<Vertex> vertices;
    std::vector<GLushort> indices;
    mLevel.generateLevel(vertices, indices);
    IMesh::Ptr levelMesh (new IndexedMeshImpl(vertices, indices));
    mResources.insert(ResourceIds::Meshes::LEVEL_ID, std::move(levelMesh));

    /************ Textures ***********************************************/
    // @TODO initialize the post processor impl texture in here (fullscreen quad isn't initialized before PP)
    ITexture::Ptr testTex (new Tex2dImpl(mSdlWindow,
        ResourcePaths::Textures::TEST_TEX_ATLAS_PATH, 0));
    mResources.insert(ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID, std::move(testTex));

    ITexture::Ptr skyboxTex (new TexSkyboxImpl(mSdlWindow,
        ResourcePaths::Textures::SKYBOX_PATHS, 0));
    mResources.insert(ResourceIds::Textures::SKYBOX_TEX_ID, std::move(skyboxTex));

    ITexture::Ptr fullScreenTex (new Tex2dImpl(
        mSdlWindow.getWindowWidth(), mSdlWindow.getWindowHeight(), 0)); // use as 1 due to Post Processor
    mResources.insert(ResourceIds::Textures::FULLSCREEN_TEX_ID, std::move(fullScreenTex));

    ITexture::Ptr charsTex (new Tex2dImpl(mSdlWindow,
        ResourcePaths::Textures::TEST_RPG_CHARS_PATH, 0));
    mResources.insert(ResourceIds::Textures::Atlas::TEST_RPG_CHARS_ID, std::move(charsTex));

    ITexture::Ptr perlinTex (new TexPerlinImpl(4.0f, 0.5f, 128, 128, true, 0));
    mResources.insert(ResourceIds::Textures::PERLIN_NOISE_2D_ID, std::move(perlinTex));

    /************** Music *************************************************/
    Music::Ptr mus (new Music(ResourcePaths::Music::WRATH_OF_SIN_MP3_PATH));
    mResources.insert(ResourceIds::Music::WRATH_OF_SIN_ID, std::move(mus));

    /************** Sound Effects ***************************************/
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

    /****************** Particles ****************************************/
    mParticles = std::move(Particle::Ptr(new Particle(
        Draw::Config(ResourceIds::Shaders::PARTICLES_SHADER_ID, "", "", ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID))));

}

/**
 * @brief Blowtorch::initPositions
 */
void Blowtorch::initPositions()
{
    mPlayer.move(mLevel.getPlayerPosition(), 1.0f);

    for (auto& enemyPos : mLevel.getEnemyPositions())
    {
        mEnemies.emplace_back(std::move(new Enemy(
            Draw::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
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
            Draw::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
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
            Draw::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
            ResourceIds::Meshes::VAO_ID,
            "",
            ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_ID,
            Utils::getTexAtlasOffset(ResourceIds::Textures::Atlas::BREAKOUT_POWER_UP_CONFUSE,
            ResourceIds::Textures::Atlas::TEST_ATLAS_TEX_NUM_ROWS)),
            pos
        )));
    }

    for (auto& pos : mLevel.getStrengthPowerUps())
    {
        mPowerUps.emplace_back(std::move(new Sprite(
            Draw::Config(ResourceIds::Shaders::SPRITE_SHADER_ID,
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
 * @brief Blowtorch::calcFrameRate
 * @param dt
 */
void Blowtorch::calcFrameRate(const float dt)
{
    ++mFrameCounter;
    mTimeSinceLastUpdate += dt;
    if (mTimeSinceLastUpdate >= 1.0f)
    {
        SDL_Log("FPS: %u\n", mFrameCounter);
        SDL_Log("time (us) / frame: %f\n", mTimeSinceLastUpdate / mFrameCounter);
        mLogger.appendToLog("FPS: " + Utils::toString(mFrameCounter));
        mLogger.appendToLog("\n");
        mLogger.appendToLog("time (us) / frame: " + Utils::toString(mTimeSinceLastUpdate / mFrameCounter));
        mLogger.appendToLog("\n");
        mFrameCounter = 0;
        mTimeSinceLastUpdate -= 1.0f;
    }
}

/**
 * @brief Blowtorch::sdlEvents
 * @param event
 * @param mouseWheelDy
 */
void Blowtorch::sdlEvents(SDL_Event& event, float& mouseWheelDy)
{
    if (event.type == SDL_QUIT)
        mPlay = false;
    else if (event.type == SDL_WINDOWEVENT)
    {
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            unsigned int newWidth = event.window.data1;
            unsigned int newHeight = event.window.data2;
            glViewport(0, 0, newWidth, newHeight);

#if defined(BLOWTORCH_DEBUG_MODE)
        std::string resizeDimens = "Resize Event -- Width: " + Utils::toString(newWidth) + ", Height: " + Utils::toString(newHeight) + "\n";
        SDL_Log(resizeDimens.c_str());
#endif // defined
        }
    }
    else if (event.type == SDL_MOUSEWHEEL)
        mouseWheelDy = event.wheel.y;
    // else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
    //     mPlayer.shoot();
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
            mPlay = false;
    }
    else if ((mSdlWindow.getInitFlags() & SDL_INIT_JOYSTICK) &&
        event.type == SDL_JOYBUTTONDOWN)
    {
#if defined(BLOWTORCH_DEBUG_MODE)
        if (event.jbutton.button == SDL_CONTROLLER_BUTTON_X &&
            mSdlWindow.hapticRumblePlay(0.75, 500) != 0)
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, SDL_GetError());
#endif // defined
    }
} // sdlEvents
