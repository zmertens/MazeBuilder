#include "AudioHandler.hpp"

#include <SDL3/SDL.h>

struct AudioHandler::AudioHandlerImpl {

    SDL_AudioSpec audioSpec;
    std::uint32_t wavLength;
    std::uint8_t* wavBuffer;
    SDL_AudioDeviceID deviceId;

    AudioHandlerImpl() : audioSpec{}, wavLength{ 0 }, wavBuffer{}, deviceId{} {

    }

    ~AudioHandlerImpl() {
        if (wavBuffer != nullptr) {
            //SDL_DestroyAudioStream(deviceId);
        }
    }
};


AudioHandler::AudioHandler() : impl(std::make_unique<AudioHandlerImpl>()) {
}

AudioHandler::~AudioHandler() = default;

bool AudioHandler::loadAudio(const std::string& audioFile) {
    if (SDL_LoadWAV(audioFile.c_str(), &impl->audioSpec, &impl->wavBuffer, &impl->wavLength)) {
        return false;
    }
    return true;
}
void AudioHandler::playAudio() const noexcept {

}

void AudioHandler::stopAudio() const noexcept {

}
