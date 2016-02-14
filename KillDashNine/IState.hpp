#ifndef ISTATE
#define ISTATE

class IState
{
public:
    virtual void changeState() const = 0;
    virtual void getState() const = 0;
};

#endif // ISTATE

