#ifndef IGAME_HPP
#define IGAME_HPP

namespace Game
{
enum class States {
	Play,
	Paused
};
} // namespace

class IGame
{
public:
    virtual void start() = 0;

protected:
    virtual void loop() = 0;
    virtual void handleEvents() = 0;
    virtual void update(float dt, double timeSinceInit) = 0;
    virtual void render() = 0;
    virtual void finish() = 0;
};

#endif // IGAME_HPP
