#ifndef IAPPLICATION_HPP
#define IAPPLICATION_HPP

class IApplication
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

#endif // IAPPLICATION_HPP
