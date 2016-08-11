#ifndef IFRAMEBUFFER
#define IFRAMEBUFFER

#include <memory>

class IFramebuffer
{
public:
    typedef std::unique_ptr<IFramebuffer> Ptr;

public:
    virtual void cleanUp() = 0;
    virtual void bind() const = 0;
    virtual void release() const = 0;
    virtual unsigned int getHandle() const = 0;

protected:
    virtual void genFrameBuffer() = 0;
    virtual void init(const unsigned int width, const unsigned int height) = 0;
};

#endif // IFRAMEBUFFER

