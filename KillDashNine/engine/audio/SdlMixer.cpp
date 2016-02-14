#include "SdlMixer.hpp"

#include "../ResourceManager.hpp"

/**
 * @brief SdlMixer::SdlMixer
 * @param rm
 */
SdlMixer::SdlMixer(const ResourceManager& rm)
: mResources(rm)
, cMaxChannelVolume(MIX_MAX_VOLUME)
, cMaxMusicVolume(MIX_MAX_VOLUME)
, cFrequency(44100)
, cFormat(MIX_DEFAULT_FORMAT)
, cChannels(2) // stereo
, cChunkSize(2048)
, mIsInitialized(true)
{
    init(MIX_INIT_MP3 | MIX_INIT_OGG);
}

/**
 * @brief SdlMixer::~SdlMixer
 */
SdlMixer::~SdlMixer()
{
    if (mIsInitialized)
    {
        cleanUp();
    }

}

/**
 * @brief SdlMixer::cleanUp
 */
void SdlMixer::cleanUp()
{
    mIsInitialized = false;

    Mix_Quit();
}

/**
 * @brief SdlMixer::setVolume
 * @param channel
 * @param volume
 */
void SdlMixer::setVolume(int channel, int volume)
{
    Mix_Volume(channel, volume);
}

/**
 * @brief SdlMixer::playChannel
 * @param channel
 * @param id
 * @param loops
 */
void SdlMixer::playChannel(int channel, const std::string& id, int loops)
{
    auto& chunk = mResources.getChunk(id);
    if (Mix_PlayChannel(channel, chunk->getChunk(), loops) == -1 && APP_DEBUG)
    {
        //SDL_LogError(SDL_LOG_CATEGORY_ERROR, Mix_GetError());
    }
}

/**
 * @brief SdlMixer::playChannelTimed
 * @param channel
 * @param id
 * @param loops
 * @param ticks
 */
void SdlMixer::playChannelTimed(int channel, const std::string& id, int loops, int ticks)
{

}

/**
 * @brief SdlMixer::fadeInChannel
 * @param channel
 * @param id
 * @param loops
 * @param ms
 */
void SdlMixer::fadeInChannel(int channel, const std::string& id, int loops, int ms)
{

}

/**
 * @brief SdlMixer::fadeInChannelTimed
 * @param channel
 * @param id
 * @param loops
 * @param ms
 * @param ticks
 */
void SdlMixer::fadeInChannelTimed(int channel, const std::string& id, int loops, int ms, int ticks)
{

}

/**
 * @brief SdlMixer::pause
 * @param channel
 */
void SdlMixer::pause(int channel)
{

}

/**
 * @brief SdlMixer::resume
 * @param channel
 */
void SdlMixer::resume(int channel)
{

}

/**
 * @brief SdlMixer::haltChannel
 * @param channel
 */
void SdlMixer::haltChannel(int channel)
{

}

/**
 * @brief SdlMixer::expireChannel
 * @param channel
 * @param ticks
 */
void SdlMixer::expireChannel(int channel, int ticks)
{

}

/**
 * @brief SdlMixer::fadeOutChannel
 * @param channel
 * @param ms
 */
void SdlMixer::fadeOutChannel(int channel, int ms)
{

}

/**
 * @brief SdlMixer::channelFinished
 */
void SdlMixer::channelFinished(void (*channelFinished)(int))
{

}

/**
 * @brief SdlMixer::playing
 * @param channel
 */
void SdlMixer::playing(int channel)
{

}

/**
 * @brief SdlMixer::paused
 * @param channel
 */
void SdlMixer::paused(int channel)
{

}

/**
 * @brief SdlMixer::fadingChannel
 * @param which
 * @return
 */
Mix_Fading SdlMixer::fadingChannel(int which)
{

}

/**
 * @brief SdlMixer::getChunk
 * @param channel
 * @return
 */
Mix_Chunk* SdlMixer::getChunk(int channel) const
{

}

/**
 * @brief SdlMixer::playMusic
 * @param id
 * @param loops
 */
void SdlMixer::playMusic(const std::string& id, int loops)
{
    auto& music = mResources.getMusic(id);
    if (Mix_PlayMusic(music->getMusic(), loops) == -1 && APP_DEBUG)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, Mix_GetError());
    }
}

/**
 * @brief SdlMixer::fadeInMusic
 * @param id
 * @param loops
 * @param ms
 */
void SdlMixer::fadeInMusic(const std::string& id, int loops, int ms)
{

}

/**
 * @brief SdlMixer::fadeInMusicPos
 * @param id
 * @param loops
 * @param ms
 * @param pos
 */
void SdlMixer::fadeInMusicPos(const std::string& id, int loops, int ms, double pos)
{

}

/**
 * @brief SdlMixer::setVolumeMusic
 * @param volume
 */
void SdlMixer::setVolumeMusic(int volume)
{

}

/**
 * @brief SdlMixer::pauseMusic
 */
void SdlMixer::pauseMusic()
{

}

/**
 * @brief SdlMixer::resumeMusic
 */
void SdlMixer::resumeMusic()
{

}

/**
 * @brief SdlMixer::rewindMusic
 */
void SdlMixer::rewindMusic()
{

}

/**
 * @brief SdlMixer::setMusicPosition
 * @param position
 */
void SdlMixer::setMusicPosition(double position)
{

}

/**
 * @brief SdlMixer::haltMusic
 */
void SdlMixer::haltMusic()
{

}

/**
 * @brief SdlMixer::fadeOutMusic
 * @param ms
 */
void SdlMixer::fadeOutMusic(int ms)
{

}

/**
 * @brief SdlMixer::getMusicType
 * @param id
 * @return
 */
Mix_MusicType SdlMixer::getMusicType(const std::string& id) const
{

}

/**
 * @brief SdlMixer::playingMusic
 * @return
 */
bool SdlMixer::playingMusic() const
{

}

/**
 * @brief SdlMixer::pausedMusic
 * @return
 */
bool SdlMixer::pausedMusic() const
{

}

/**
 * @brief SdlMixer::fadingMusic
 * @return
 */
Mix_Fading SdlMixer::fadingMusic() const
{

}

/**
 * @brief SdlMixer::setPanning
 * @param channel
 * @param left
 * @param right
 */
void SdlMixer::setPanning(int channel, Uint8 left, Uint8 right)
{

}

/**
 * @brief SdlMixer::setDistance
 * @param channel
 * @param distance
 */
void SdlMixer::setDistance(int channel, Uint8 distance)
{

}

/**
 * @brief SdlMixer::setPosition
 * @param channel
 * @param angle
 * @param distance
 */
void SdlMixer::setPosition(int channel, Sint16 angle, Uint8 distance)
{

}

/**
 * @brief SdlMixer::setReverseStereo
 * @param channel
 * @param flip
 */
void SdlMixer::setReverseStereo(int channel, int flip)
{

}

/**
 * @brief SdlMixer::getMaxChannelVolume
 * @return
 */
int SdlMixer::getMaxChannelVolume() const
{
    return cMaxChannelVolume;
}

/**
 * @brief SdlMixer::getMaxMusicVolume
 * @return
 */
int SdlMixer::getMaxMusicVolume() const
{
    return cMaxMusicVolume;
}

/**
 * @brief SdlMixer::init
 * @param flags
 */
void SdlMixer::init(Uint32 flags)
{
    Mix_Init(flags);
    if (Mix_OpenAudio(cFrequency, cFormat, cChannels, cChunkSize) < 0 && APP_DEBUG)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, Mix_GetError());
    }
}

