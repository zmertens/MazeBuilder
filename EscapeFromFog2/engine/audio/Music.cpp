#include "Music.hpp"

/**
 * @brief Music::Music
 * @param path
 */
Music::Music(const std::string& path)
{
    mMusic = Mix_LoadMUS(path.c_str());
    if (!mMusic)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, Mix_GetError());
    }
}

/**
 * @brief Music::~Music
 */
Music::~Music()
{
    if (mMusic)
        cleanUp();
}

/**
 * @brief Music::cleanUp
 */
void Music::cleanUp()
{
    Mix_FreeMusic(mMusic);
    mMusic = nullptr;
}

/**
 * @brief Music::getMusic
 * @return
 */
Mix_Music* Music::getMusic() const
{
    return mMusic;
}

