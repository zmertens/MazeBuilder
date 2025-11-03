#ifndef AUDIOHELPER_HPP
#define AUDIOHELPER_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include <SFML/Audio.hpp>

class AudioHelper
{
public:
    /// @brief 
    /// @param audioConfig 
    explicit AudioHelper(const std::unique_ptr<sf::Sound>& generateSound) noexcept;

    /// @brief 
    ~AudioHelper();

    // Non-copyable, movable
    AudioHelper(const AudioHelper&) = delete;
    AudioHelper& operator=(const AudioHelper&) = delete;
    AudioHelper(AudioHelper&&) noexcept;
    AudioHelper& operator=(AudioHelper&&) noexcept;

    // Play a short sound effect by name (from config)
    void playSound(const std::string& name);

    // Play a music track by name (from config)
    void playMusic(const std::string& name, bool loop = false);

    // Stop currently playing music
    void stopMusic();

    // Set music volume (0-100)
    void setMusicVolume(float volume);

    // Set sound effect volume (0-100)
    void setSoundVolume(float volume);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // AUDIOHELPER_HPP
