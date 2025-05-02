#ifndef SDLHELPER_HPP
#define SDLHELPER_HPP

#include <cstdint>
#include <string>
#include <string_view>

#include "State.hpp"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_AudioStream;

struct OrthographicCamera;

class SDLHelper {
public:
    SDL_Window* window;

    SDL_Renderer* renderer;

private:
    std::uint32_t audioDeviceId;
    SDL_AudioStream* audioStream;
    std::uint8_t* wavBuffer;
    std::uint32_t wavLength;

public:
    explicit SDLHelper() noexcept;
    ~SDLHelper();

    void init() noexcept;

    bool loadFont(const std::string& f, unsigned int fSize) noexcept;

    void poll_events(State& state, OrthographicCamera& camera) noexcept;

    void updateAudioData() noexcept;

    void playAudioStream() noexcept;

    void pauseAudioStream() noexcept;

    void stopAudioStream() noexcept;

    bool loadWAV(std::string_view path) noexcept;
}; // SDLHelper class

#endif // SDLHELPER_HPP
