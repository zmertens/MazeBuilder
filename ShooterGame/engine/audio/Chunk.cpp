#include "Chunk.hpp"

/**
 * @brief Chunk::getChunk
 * @return
 */
Chunk::Chunk(const std::string& path)
{
    mChunk = Mix_LoadWAV(path.c_str());
    if (!mChunk)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, Mix_GetError());
    }
}

/**
 * @brief Chunk::~Chunk
 */
Chunk::~Chunk()
{
    if (mChunk)
        cleanUp();
}

/**
 * @brief Chunk::cleanUp
 */
void Chunk::cleanUp()
{
    Mix_FreeChunk(mChunk);
    mChunk = nullptr;
}

/**
 * @brief Chunk::getChunk
 * @return
 */
Mix_Chunk* Chunk::getChunk() const
{
    return mChunk;
}

