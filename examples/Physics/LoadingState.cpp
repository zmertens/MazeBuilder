#include "LoadingState.hpp"

#include <SDL3/SDL.h>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>

#include "JsonUtils.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "StateStack.hpp"
#include "Texture.hpp"

/// @brief 
/// @param stack 
/// @param context 
/// @param resourcePath ""
LoadingState::LoadingState(StateStack& stack, State::Context context, std::string_view resourcePath)
    : State(stack, context)
    , mLoadingSprite{context.textures->get(Textures::ID::MAZE)}
    , mForeman{}
    , mHasFinished{false} {

    mForeman.initThreads();
    
    // Start loading resources in background if path is provided
    if (!resourcePath.empty()) {

        loadResources(resourcePath);
    } else {

        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "LoadingState: No resource path provided: %s\n", resourcePath.data());
        mHasFinished = true;
    }
}

void LoadingState::draw() const noexcept {

    auto& window = *getContext().window;

    window.draw(mLoadingSprite);
}

bool LoadingState::update(float dt) noexcept {

    if (!mHasFinished && mForeman.isDone()) {
        
        // Loading is complete - get the loaded resources
        auto resources = mForeman.getResources();
        SDL_Log("Loading complete! Loaded %zu resources. Loading textures...\n", resources.size());
        
        // Now actually load the textures from the resource metadata
        loadTexturesFromResources(resources);
        
        mHasFinished = true;
        SDL_Log("All textures loaded! Press any key to continue...\n");
    }

    if (!mHasFinished) {

        setCompletion(mForeman.getCompletion());
    }

    return true;
}

bool LoadingState::handleEvent(const SDL_Event& event) noexcept {

	return true;
}

void LoadingState::setCompletion(float percent) noexcept {
    
    if (percent > 1.f) {
        percent = 1.f;
    }

    // Update loading sprite or progress bar based on percent
    SDL_Log("Loading progress: %.2f%%", percent * 100.f);
}

bool LoadingState::isFinished() const noexcept { return mHasFinished; }

/// @brief Load resources from the specified path
/// @param resourcePath Path to the JSON resource configuration
void LoadingState::loadResources(std::string_view resourcePath) noexcept {

    SDL_Log("LoadingState::loadResources - Loading from: %s\n", resourcePath.data());
    
    // This would be called by the application to trigger resource loading
    // The resources would be loaded by the worker threads and stored
    mForeman.generate(resourcePath);
}

void LoadingState::loadTexturesFromResources(const std::unordered_map<std::string, std::string>& resources) noexcept {
    using std::string;
    
    auto& textures = *getContext().textures;
    JsonUtils jsonUtils{};
    
    try {
        // Load splash screen texture
        auto splashImageKey = resources.find("splash_image");
        if (splashImageKey != resources.end()) {
            string splashImagePath = "resources/" + jsonUtils.extractJsonValue(splashImageKey->second);
            SDL_Log("DEBUG: Loading splash screen from: %s", splashImagePath.c_str());
            textures.load(Textures::ID::SPLASH_SCREEN, splashImagePath);
        }
        
        // Load avatar texture
        auto avatarKey = resources.find("avatar");
        if (avatarKey != resources.end()) {
            string avatarImagePath = "resources/" + jsonUtils.extractJsonValue(avatarKey->second);
            SDL_Log("DEBUG: Loading avatar from: %s", avatarImagePath.c_str());
            textures.load(Textures::ID::AVATAR, avatarImagePath);
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Avatar resource not found, using default\n");
            textures.load(Textures::ID::AVATAR, "resources/character_beige_front.png");
        }
        
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load textures from resources: %s", e.what());
    }
}