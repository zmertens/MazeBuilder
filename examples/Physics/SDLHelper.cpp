#include "SDLHelper.hpp"

#include <SDL3/SDL.h>

#include <iostream>

#include "OrthographicCamera.hpp"
    
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

void SDLHelper::init() noexcept {
    using namespace std;
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init success\n");

        // Audio initialization moved to a separate method that gets called
        // only after SDL_Init has succeeded and a WAV file has been loaded
    } else {
        cerr << "SDL_Init Error: " << SDL_GetError() << endl;
    }
}

void SDLHelper::poll_events(State& state, std::unique_ptr<OrthographicCamera> const& camera) noexcept {
    using namespace std;

    SDL_Event e;
    
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_KEY_DOWN) {
            // Handle splash screen transition - any key press transitions from SPLASH to MAIN_MENU
            if (state == State::SPLASH) {
                state = State::MAIN_MENU; // Go to main menu for maze creation
                break;
            }
            
            if (e.key.scancode == SDL_SCANCODE_ESCAPE) {
                state = State::DONE;
                break;
            } else if (e.key.scancode == SDL_SCANCODE_B) {
                // Generate maze when in MAIN_MENU state
                if (state == State::MAIN_MENU) {
                    state = State::PLAY_SINGLE_MODE;
                }
            } 
            // Arrow key controls for camera movement
            else if (e.key.scancode == SDL_SCANCODE_LEFT) {
                camera->x += camera->panSpeed * 2.0f;
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Camera moved left: x=%.2f, y=%.2f", camera->x, camera->y);
            }
            else if (e.key.scancode == SDL_SCANCODE_RIGHT) {
                camera->x -= camera->panSpeed * 2.0f;
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Camera moved right: x=%.2f, y=%.2f", camera->x, camera->y);
            }
            else if (e.key.scancode == SDL_SCANCODE_UP) {
                camera->y += camera->panSpeed * 2.0f;
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Camera moved up: x=%.2f, y=%.2f", camera->x, camera->y);
            }
            else if (e.key.scancode == SDL_SCANCODE_DOWN) {
                camera->y -= camera->panSpeed * 2.0f;
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Camera moved down: x=%.2f, y=%.2f", camera->x, camera->y);
            }
            // Rotation controls with Q/E keys
            else if (e.key.scancode == SDL_SCANCODE_Q) {
                camera->rotation -= camera->rotationSpeed * 2.0f;
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Camera rotated counter-clockwise: rotation=%.2f", camera->rotation);
            }
            else if (e.key.scancode == SDL_SCANCODE_E) {
                camera->rotation += camera->rotationSpeed * 2.0f;
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Camera rotated clockwise: rotation=%.2f", camera->rotation);
            }
            // Zoom controls with +/- (equals/minus) keys
            else if (e.key.scancode == SDL_SCANCODE_EQUALS) { // + key (may require shift)
                camera->zoom *= (1.0f + camera->zoomSpeed);
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Camera zoomed in: zoom=%.2f", camera->zoom);
            }
            else if (e.key.scancode == SDL_SCANCODE_MINUS) { // - key
                camera->zoom /= (1.0f + camera->zoomSpeed);
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Camera zoomed out: zoom=%.2f", camera->zoom);
            }
            // Reset camera with R key
            else if (e.key.scancode == SDL_SCANCODE_R) {
                camera->x = 0.0f;
                camera->y = 0.0f;
                camera->zoom = 1.0f;
                camera->rotation = 0.0f;
                SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Camera reset to default position and orientation");
            }
        } else if (e.type == SDL_EVENT_QUIT) {
            state = State::DONE;
            break;
        }
    }
    
    // Also check for continuous key presses for smoother camera movement
    const auto* keyState = SDL_GetKeyboardState(NULL);
    if (keyState[SDL_SCANCODE_LEFT]) {
        camera->x += camera->panSpeed;
    }
    if (keyState[SDL_SCANCODE_RIGHT]) {
        camera->x -= camera->panSpeed;
    }
    if (keyState[SDL_SCANCODE_UP]) {
        camera->y += camera->panSpeed;
    }
    if (keyState[SDL_SCANCODE_DOWN]) {
        camera->y -= camera->panSpeed;
    }
    
    // Add continuous rotation and zoom (for smoother control)
    if (keyState[SDL_SCANCODE_Q]) {
        camera->rotation -= camera->rotationSpeed;
    }
    if (keyState[SDL_SCANCODE_E]) {
        camera->rotation += camera->rotationSpeed;
    }
    if (keyState[SDL_SCANCODE_EQUALS]) { // + key
        camera->zoom *= (1.0f + camera->zoomSpeed * 0.005f); // gentler continuous zoom
    }
    if (keyState[SDL_SCANCODE_MINUS]) { // - key
        camera->zoom /= (1.0f + camera->zoomSpeed * 0.005f); // gentler continuous zoom
    }
    
    // Enforce zoom limits to prevent extreme values
    camera->zoom = SDL_max(0.1f, SDL_min(5.0f, camera->zoom));
}

void SDLHelper::updateAudioData() noexcept {
    // Only attempt audio operations if we have valid audio data
    if (!this->wavBuffer || !this->wavLength || !this->audioStream) {
        return;
    }

    // Check if we need to add more data to the stream
    if (SDL_GetAudioStreamAvailable(this->audioStream) < this->wavLength) {
        SDL_PutAudioStreamData(this->audioStream, this->wavBuffer, this->wavLength);
        auto streamingBytesAvailable = SDL_GetAudioStreamAvailable(this->audioStream);
        
        if (streamingBytesAvailable < this->wavLength) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Audio stream buffer is not full: %s\n", SDL_GetError());
        } else {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Audio stream buffer is full: %u bytes available", streamingBytesAvailable);
        }
    }
}

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
