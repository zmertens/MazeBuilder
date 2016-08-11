#ifndef SDLMIXER_HPP
#define SDLMIXER_HPP

#include <string>

#include "../SdlManager.hpp"

class ResourceManager;

/**
 * https://www.libsdl.org/projects/SDL_mixer/docs/SDL_mixer_toc.html#SEC_Contents
 * @note: NEVER call SDL_Mixer functions, nor SDL_LockAudio, from a callback function.
 * @brief The SdlMixer class
 */
class SdlMixer
{
public:
    SdlMixer(const ResourceManager& rm);
    virtual ~SdlMixer();
    void cleanUp();

    // channels
    void setVolume(int channel, int volume);
    void playChannel(int channel, const std::string& id, int loops);
    void playChannelTimed(int channel, const std::string& id, int loops, int ticks);
    void fadeInChannel(int channel, const std::string& id, int loops, int ms);
    void fadeInChannelTimed(int channel, const std::string& id, int loops, int ms, int ticks);
    void pause(int channel);
    void resume(int channel);
    void haltChannel(int channel);
    void expireChannel(int channel, int ticks);
    void fadeOutChannel(int channel, int ms);
    void channelFinished(void (*channelFinished) (int channel));
    void playing(int channel);
    void paused(int channel);
    Mix_Fading fadingChannel(int which);
    Mix_Chunk* getChunk(int channel) const;

    // music
    void playMusic(const std::string& id, int loops);
    void fadeInMusic(const std::string& id, int loops, int ms);
    void fadeInMusicPos(const std::string& id, int loops, int ms, double pos);
    void setVolumeMusic(int volume);
    void pauseMusic();
    void resumeMusic();
    void rewindMusic();
    void setMusicPosition(double position);
    void haltMusic();
    void fadeOutMusic(int ms);
    Mix_MusicType getMusicType(const std::string& id) const;
    bool playingMusic() const;
    bool pausedMusic() const;
    Mix_Fading fadingMusic() const;

    // effects
    void setPanning(int channel, Uint8 left, Uint8 right);
    void setDistance(int channel, Uint8 distance);
    void setPosition(int channel, Sint16 angle, Uint8 distance);
    void setReverseStereo(int channel, int flip);

    int getMaxChannelVolume() const;
    int getMaxMusicVolume() const;

private:
    const ResourceManager& mResources;
    const int cMaxChannelVolume;
    const int cMaxMusicVolume;
    const int cFrequency;
    const Uint16 cFormat;
    const int cChannels;
    const int cChunkSize;
    bool mIsInitialized;

private:
    SdlMixer(const SdlMixer& other);
    SdlMixer& operator=(const SdlMixer& other);
    void init(Uint32 flags);
};

#endif // SDLMIXER_HPP
