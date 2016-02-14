#ifndef TEX2DIMPL_HPP
#define TEX2DIMPL_HPP

#include "ITexture.hpp"

#include "../SdlManager.hpp"

class Tex2dImpl : public ITexture
{
public:
    explicit Tex2dImpl(const SdlManager& sdlManager,
        const std::string& fileName,
        unsigned int channel = 0);
    explicit Tex2dImpl(const unsigned int width, const unsigned int height,
        unsigned int channel = 0);

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
};

#endif // TEX2DIMPL_HPP
