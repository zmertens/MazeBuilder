#include "SDLHelper.hpp"

#include <SDL3/SDL.h>

#include <iostream>
    
SDLHelper::SDLHelper() noexcept
: window{ nullptr }, renderer{ nullptr }, audioStream{}, wavBuffer{}, wavLength{} {

}

SDLHelper::~SDLHelper() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Audio device only cleaned if it is played/binded
    SDL_AudioDeviceID castedId = static_cast<SDL_AudioDeviceID>(this->audioDeviceId);
    if (castedId) {
        SDL_free(this->wavBuffer);
        SDL_DestroyAudioStream(this->audioStream);
        SDL_CloseAudioDevice(castedId);
    }

    SDL_Quit();
}

void SDLHelper::createWindowAndRenderer(std::string_view title, int width, int height) noexcept {

    this->window = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_RESIZABLE);

    if (!this->window) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindow failed: %s\n", SDL_GetError());

        return;
    }

    this->renderer = SDL_CreateRenderer(this->window, nullptr);

    if (!this->renderer) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(this->window);

        return;
    }

    if (auto props = SDL_GetRendererProperties(this->renderer); props != 0) {

        SDL_Log("Renderer created: %s\n", SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "default"));
    } else {
        
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get renderer info: %s\n", SDL_GetError());

        return;
    }

    SDL_SetRenderVSync(renderer, 1);
}

void SDLHelper::init() noexcept {
    using std::cerr;
    using std::endl;
    
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {

        cerr << "SDL_Init Error: " << SDL_GetError() << endl;
    }
}

void SDLHelper::updateAudioData() noexcept {
    // Only attempt audio operations if we have valid audio data
    if (!this->wavBuffer || !this->wavLength || !this->audioStream) {
        return;
    }

    // Check if we need to add more data to the stream
    if (SDL_GetAudioStreamAvailable(this->audioStream) < static_cast<int>(this->wavLength)) {
        SDL_PutAudioStreamData(this->audioStream, this->wavBuffer, this->wavLength);
        auto streamingBytesAvailable = SDL_GetAudioStreamAvailable(this->audioStream);

        if (streamingBytesAvailable < static_cast<int>(this->wavLength)) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Audio stream buffer is not full: %s\n", SDL_GetError());
        } else {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Audio stream buffer is full: %u bytes available", streamingBytesAvailable);
        }
    }
}

// Audio Handling reference from SDL_AUDIO_STREAM: SDL\test\testaudio.c
void SDLHelper::playAudioStream() noexcept {
    if (this->audioDeviceId) {
        SDL_BindAudioStream(this->audioDeviceId, this->audioStream);
        SDL_ResumeAudioStreamDevice(this->audioStream);
    }
}

void SDLHelper::pauseAudioStream() noexcept {
    if (this->audioDeviceId) {
        SDL_PauseAudioStreamDevice(this->audioStream);
    }
}

void SDLHelper::stopAudioStream() noexcept {
    if (this->audioDeviceId) {
        SDL_UnbindAudioStream(this->audioStream);
        SDL_FlushAudioStream(this->audioStream);
    }
}

bool SDLHelper::loadWAV(std::string_view path) noexcept {
    SDL_AudioSpec audioSpec;

    // Clear any previous audio data
    if (this->wavBuffer) {
        SDL_free(this->wavBuffer);
        this->wavBuffer = nullptr;
        this->wavLength = 0;
    }

    // Close any existing audio resources
    if (this->audioDeviceId) {
        SDL_CloseAudioDevice(static_cast<SDL_AudioDeviceID>(this->audioDeviceId));
        this->audioDeviceId = 0;
    }
    if (this->audioStream) {
        SDL_DestroyAudioStream(this->audioStream);
        this->audioStream = nullptr;
    }

    // Load the WAV file data
    if (!SDL_LoadWAV(path.data(), &audioSpec, &wavBuffer, &wavLength)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load WAV file: %s\n%s\n", path.data(), SDL_GetError());
        return false;
    }

    // Now that we have valid audio data, setup the audio stream
    this->audioStream = SDL_CreateAudioStream(&audioSpec, &audioSpec);
    if (!this->audioStream) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create audio stream: %s\n", SDL_GetError());
        SDL_free(wavBuffer);
        wavBuffer = nullptr;
        wavLength = 0;
        return false;
    }

    // Open the audio device
    this->audioDeviceId = static_cast<uint32_t>(SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr));
    if (!this->audioDeviceId) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to open audio device: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(this->audioStream);
        audioStream = nullptr;
        SDL_free(wavBuffer);
        wavBuffer = nullptr;
        wavLength = 0;
        return false;
    }

    // Load the initial audio data into the stream
    SDL_PutAudioStreamData(this->audioStream, this->wavBuffer, this->wavLength);
    SDL_FlushAudioStream(this->audioStream);

    auto stream_bytes_len = SDL_GetAudioStreamAvailable(this->audioStream);
    if (stream_bytes_len != this->wavLength) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Audio stream has incorrect data length: %u vs expected %u\n", 
                    stream_bytes_len, this->wavLength);
        return false;
    }

    return true;
}
