#ifndef AUDIOHANDLER_HPP
#define AUDIOHANDLER_HPP

#include <memory>
#include <string>

class AudioHandler {
public:
    AudioHandler();
    ~AudioHandler();

    bool loadAudio(const std::string& audioFile);
    void playAudio() const noexcept;
    void stopAudio() const noexcept;
private:
    struct AudioHandlerImpl;
    std::unique_ptr<AudioHandlerImpl> impl;
};

#endif // AUDIOHANDLER_HPP
