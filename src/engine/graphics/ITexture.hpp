#ifndef ITEXTURE
#define ITEXTURE

#include <string>
#include <vector>
#include <memory>

class ITexture
{
public:
    typedef std::unique_ptr<ITexture> Ptr;

public:
    virtual void cleanUp() = 0;
    virtual void bind() const = 0;
    virtual void release() const = 0;
    virtual unsigned int getHandle() const = 0;

protected:
    virtual void genTexture() = 0;
    virtual void init(unsigned char* str, long bufferSize) = 0;
    virtual void init(const unsigned int width, const unsigned int height) = 0;
};

#endif // ITEXTURE

