#ifndef TEXPERLINNOISE2DIMPL_HPP
#define TEXPERLINNOISE2DIMPL_HPP

#include "ITexture.hpp"

#include "../SdlManager.hpp"

class TexPerlinNoise2dImpl : public ITexture
{
public:
    explicit TexPerlinNoise2dImpl(const float baseFrequency,
        const float persistence, const int width, const int height,
        bool periodic, unsigned int channel = 0);

    virtual void cleanUp() override;
    virtual void bind() const override;
    virtual void release() const override;
    virtual unsigned int getHandle() const override;

protected:
    virtual void genTexture() override;
    virtual void init(unsigned char* str, long bufferSize) override;
    virtual void init(const unsigned int width, const unsigned int height) override;

private:
    GLuint mChannel;
    GLenum mTarget;
    GLenum mInternalFormat;
    GLenum mPixelFormat;
    GLenum mWrapS;
    GLenum mWrapT;
    GLenum mMinFilter;
    GLenum mMagFilter;
    GLuint mHandle;
    GLubyte* mTexData;
};

#endif // TEXPERLINNOISE2DIMPL_HPP
