#ifndef GAMEIMPL_HPP
#define GAMEIMPL_HPP

#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

#include "IGame.hpp"

#include "engine/SdlWindow.hpp"
#include "engine/ResourceManager.hpp"
#include "engine/Logger.hpp"
#include "engine/Camera.hpp"
#include "engine/ImGuiHelper.hpp"

#include "engine/graphics/IDrawable.hpp"
#include "engine/graphics/Skybox.hpp"
#include "engine/graphics/Light.hpp"
#include "engine/graphics/PostProcessorImpl.hpp"
#include "engine/graphics/Sprite.hpp"

//#include "engine/audio/SdlMixer.hpp"

#include "Level.hpp"
#include "Enemy.hpp"
#include "Player.hpp"
#include "Useless.hpp"

class GameImpl final : public IGame
{
    friend class ImGuiHelper;
public:
    GameImpl();
    virtual void start() override;

protected:
    virtual void loop() override;
    virtual void handleEvents() override;
    virtual void update(float dt, double timeSinceInit) override;
    virtual void render() override;
    virtual void finish() override;

private:
    static constexpr float sTimePerFrame = 1.0f / 60.0f;
    static constexpr unsigned int sWindowWidth = 1080u;
    static constexpr unsigned int sWindowHeight = 720u;
    static constexpr auto sTitle = "Maze";

    static std::unordered_map<uint8_t, bool> sKeyInputs;

    SdlWindow mSdlWindow;
    ResourceManager mResources;
    Logger mLogger;

    bool mPlay;
    unsigned int mFrameCounter;
    float mTimeSinceLastUpdate;
    float mAccumulator;

    ImGuiHelper mImGui;
    //SdlMixer mSdlMixer;
    Camera mCamera;
    Level mLevel;
    Player mPlayer;

    Useless mCube;
    Skybox mSkybox;
    Light mLight;
    Sprite mExitSprite;

    std::vector<Enemy::Ptr> mEnemies;
    std::vector<Sprite::Ptr> mPowerUps;
    PostProcessorImpl::Ptr mPostProcessor;

private:
    void init();
    void initResources();
    void initPositions();
    void calcFrameRate(const float dt);
    void sdlEvents(SDL_Event& event, float& mouseWheelDy);
};

#endif // GAMEIMPL_HPP
