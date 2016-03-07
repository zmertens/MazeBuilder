#ifndef KILLDASHNINE_HPP
#define KILLDASHNINE_HPP

#include <unordered_map>

#include <glm/glm.hpp>

#include "engine/SdlManager.hpp"
#include "engine/ResourceManager.hpp"
#include "engine/DataLogger.hpp"

#include "IApplication.hpp"

// temp testing
#include "engine/Camera.hpp"
#include "engine/graphics/Entity.hpp"
#include "engine/graphics/Skybox.hpp"
#include "engine/graphics/Light.hpp"
#include "engine/graphics/PostProcessorImpl.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/audio/SdlMixer.hpp"

#include "LevelGenerator.hpp"
#include "Enemy.hpp"
#include "Player.hpp"
#include "ImGuiHelper.hpp"

class KillDashNine final : public IApplication
{
public:
    KillDashNine();
    virtual void start() override;

protected:
    virtual void loop() override;
    virtual void handleEvents() override;
    virtual void update(float dt, double timeSinceInit) override;
    virtual void render() override;
    virtual void finish() override;

private:
    static const float sTimePerFrame;
    static const unsigned int sWindowWidth;
    static const unsigned int sWindowHeight;
    static const std::string sTitle;
    static std::unordered_map<uint8_t, bool> sKeyInputs;

    SdlManager mSdlManager;
    ResourceManager mResources;
    DataLogger mLogger;

    bool mAppIsRunning;
    unsigned int mFrameCounter;
    float mTimeSinceLastUpdate;
    float mAccumulator;

    ImGuiHelper mImGuiHelper;
    Entity mCube;
    Camera mCamera;
    LevelGenerator mLevelGen;
    Player mPlayer;
    Skybox mSkybox;
    PostProcessorImpl mPostProcessor;
    Light mLight;
    Sprite mTestSprite;
    std::vector<Enemy::Ptr> mEnemies;
    std::vector<Sprite::Ptr> mPowerUps;

    // exits
    // power ups

    SdlMixer mSdlMixer;

private:
    void init();
    void initResources();
    void initPositions();
    void printFramesToConsole(const float dt);
    void sdlEvents(SDL_Event& event, float& mouseWheelDy);
    void handleReturnPressed();
};

#endif // KILLDASHNINE_HPP
