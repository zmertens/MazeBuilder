#ifndef PLAYSTATE_HPP
#define PLAYSTATE_HPP

#include "IState.hpp"

class PlayState : public IState
{
public:
    explicit PlayState();
    virtual void input() override;
    virtual void update(float dt, double timeSinceInit) override;
    virtual void render() const override;
};

#endif // PLAYSTATE_HPP
