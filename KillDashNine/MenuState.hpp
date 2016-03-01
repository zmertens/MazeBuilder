#ifndef MENUSTATE_HPP
#define MENUSTATE_HPP

#include "IState.hpp"

class MenuState : public IState
{
public:
    explicit MenuState();
    virtual void input() override;
    virtual void update(float dt, double timeSinceInit) override;
    virtual void render() const override;
};

#endif // MENUSTATE_HPP
