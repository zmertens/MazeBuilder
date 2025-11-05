#include "AudioHelper.hpp"

class AudioHelper::Impl
{
    const std::unique_ptr<sf::Sound>& mSound;

public:
    Impl(const std::unique_ptr<sf::Sound>& generateSound);
    ~Impl();

    void playSound(const std::string& name);
    void playMusic(const std::string& name, bool loop);
    void stopMusic();
    void setMusicVolume(float volume);
    void setSoundVolume(float volume);

    // Add private members for SFML sound/music objects and config here
};

AudioHelper::Impl::Impl(const std::unique_ptr<sf::Sound>& generateSound)
    : mSound(generateSound)
{
    // Load sound/music files and set up the audio system here
    // For example, load sound buffers and assign them to mSound
    // mSound.setBuffer(soundBuffer);
    // mSound.setVolume(100.0f); // Set initial volume to 100%
}

AudioHelper::Impl::~Impl() = default;

void AudioHelper::Impl::playSound(const std::string& name)
{
    mSound->play();
}

void AudioHelper::Impl::playMusic(const std::string& /*name*/, bool /*loop*/)
{
    // TODO: Play music by name, set looping
}

void AudioHelper::Impl::stopMusic()
{
    // TODO: Stop music
}

void AudioHelper::Impl::setMusicVolume(float /*volume*/)
{
    // TODO: Set music volume
}

void AudioHelper::Impl::setSoundVolume(float /*volume*/)
{
    // TODO: Set sound effect volume
}

// AudioHelper public API

AudioHelper::AudioHelper(const std::unique_ptr<sf::Sound>& s) noexcept
    : impl_(std::make_unique<Impl>(s))
{
}

AudioHelper::~AudioHelper() = default;

AudioHelper::AudioHelper(AudioHelper&&) noexcept = default;
AudioHelper& AudioHelper::operator=(AudioHelper&&) noexcept = default;

void AudioHelper::playSound(const std::string& name)
{
    impl_->playSound(name);
}

void AudioHelper::playMusic(const std::string& name, bool loop)
{
    impl_->playMusic(name, loop);
}

void AudioHelper::stopMusic()
{
    impl_->stopMusic();
}

void AudioHelper::setMusicVolume(float volume)
{
    impl_->setMusicVolume(volume);
}

void AudioHelper::setSoundVolume(float volume)
{
    impl_->setSoundVolume(volume);
}
