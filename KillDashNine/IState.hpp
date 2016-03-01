#ifndef ISTATE
#define ISTATE

#include <memory>

class IState
{
public:
    typedef std::unique_ptr<IState> Ptr;

public:
    virtual void input() = 0;
    virtual void update(float dt, double timeSinceInit) = 0;
    virtual void render() const = 0;
};

#endif // ISTATE

