#include "LoadingState.hpp"

#include <SDL3/SDL.h>

#include <MazeBuilder/io_utils.h>

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
    , mLoadingSprite{context.textures->get(Textures::ID::SPLASH_SCREEN)}
    , mForeman{}
    , mHasFinished{false}
    , mResourcePath{resourcePath} {

    mForeman.initThreads();
    
    // Start loading resources in background if path is provided
    if (!mResourcePath.empty()) {

        loadResources();
    } else {

        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "LoadingState: No resource path provided: %s\n", mResourcePath.data());
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
        loadTexturesFromResources(std::cref(resources));
        
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
void LoadingState::loadResources() noexcept {

    SDL_Log("LoadingState::loadResources - Loading from: %s\n", mResourcePath.data());

    // This would be called by the application to trigger resource loading
    // The resources would be loaded by the worker threads and stored
    mForeman.generate(mResourcePath);
}

void LoadingState::loadTexturesFromResources(const std::unordered_map<std::string, std::string>& resources) noexcept {
    using std::string;
    
    auto& textures = *getContext().textures;
    JsonUtils jsonUtils{};
    
    try {

        // Construct a string from the resource path
        auto resourcePathPrefix = mazes::io_utils::getDirectoryPath(mResourcePath) + "/";

        // Load splash screen texture
        if (auto sdlBlocksKey = resources.find("sdl_blocks"); sdlBlocksKey != resources.cend()) {

            string sdlBlocksPath = resourcePathPrefix + jsonUtils.extractJsonValue(sdlBlocksKey->second);
            textures.load(Textures::ID::SDL_BLOCKS, sdlBlocksPath);
            SDL_Log("DEBUG: Loading SDL blocks from: %s", sdlBlocksPath.c_str());
        }
        
        // Load avatar texture
        if (auto avatarKey = resources.find("astronaut"); avatarKey != resources.cend()) {

            string avatarImagePath = resourcePathPrefix + jsonUtils.extractJsonValue(avatarKey->second);
            textures.load(Textures::ID::ASTRONAUT, avatarImagePath);
            SDL_Log("DEBUG: Loading astronaut from: %s", avatarImagePath.c_str());
        }
        
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load textures from resources: %s", e.what());
    }
}