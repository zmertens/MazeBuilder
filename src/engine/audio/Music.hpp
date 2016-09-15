#ifndef MUSIC_HPP
#define MUSIC_HPP

#include <string>
#include <memory>

#include "../SdlManager.hpp"

class Music
{
public:
    typedef std::unique_ptr<Music> Ptr;
public:
    explicit Music(const std::string& path);
    virtual ~Music();

    void cleanUp();

    Mix_Music* getMusic() const;

private:
    Mix_Music* mMusic;
private:
    Music(const Music& other);
    Music& operator=(const Music& other);
};

#endif // MUSIC_HPP
