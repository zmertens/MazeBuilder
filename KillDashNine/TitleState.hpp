#ifndef TITLESTATE_HPP
#define TITLESTATE_HPP

#include "IState.hpp"

class TitleState : public IState
{
public:
    TitleState();
    virtual void input() override;
    virtual void update(float dt, double timeSinceInit) override;
    virtual void render() const override;
};

#endif // TITLESTATE_HPP
