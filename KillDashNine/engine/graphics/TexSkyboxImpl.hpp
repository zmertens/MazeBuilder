#ifndef TEXSKYBOXIMPL_HPP
#define TEXSKYBOXIMPL_HPP

#include "ITexture.hpp"

#include "../SdlManager.hpp"

class TexSkyboxImpl : public ITexture
{
public:
    explicit TexSkyboxImpl(const SdlManager& sdlManager,
        const std::vector<std::string>& fileNames,
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
    static GLuint sCubeMapIndex;
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

#endif // TEXSKYBOXIMPL_HPP
