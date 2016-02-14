#include "Tex2dImpl.hpp"

#if APP_DEBUG == 1
#include "GlUtils.hpp"
#endif

#include "../Utils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../../extlibs/stb_image.h"

Tex2dImpl::Tex2dImpl(const SdlManager& sdlManager,
    const std::string& fileName,
    unsigned int channel)
: mChannel(channel)
, mTarget(GL_TEXTURE_2D)
, mInternalFormat(GL_RGBA8)
, mPixelFormat(GL_RGBA)
, mWrapS(GL_CLAMP_TO_EDGE)
, mWrapT(GL_CLAMP_TO_EDGE)
, mMinFilter(GL_NEAREST_MIPMAP_LINEAR)
, mMagFilter(GL_NEAREST)
, mHandle(0)
{
    genTexture();

    long bufferSize;
    unsigned char* str = sdlManager.buildBufferFromFile(fileName, bufferSize);

    init(str, bufferSize);
}

/**
 * @brief Tex2dImpl::Tex2dImpl
 * @param width
 * @param height
 * @param channel = 0
 */
Tex2dImpl::Tex2dImpl(const unsigned int width,
    const unsigned int height,
    unsigned int channel)
: mChannel(channel)
, mTarget(GL_TEXTURE_2D)
, mInternalFormat(GL_RGBA8)
, mPixelFormat(GL_RGBA)
, mWrapS(GL_CLAMP_TO_EDGE)
, mWrapT(GL_CLAMP_TO_EDGE)
, mMinFilter(GL_NEAREST)
, mMagFilter(GL_NEAREST)
, mHandle(0)
{
    genTexture();
    init(width, height);
}

/**
 * @brief Tex2dImpl::cleanUp
 */
void Tex2dImpl::cleanUp()
{
    glDeleteTextures(1, &mHandle);
}

void Tex2dImpl::bind() const
{
    glBindTextureUnit(mChannel, mHandle);
}

void Tex2dImpl::release() const
{
    glBindTexture(mTarget, 0);
}

unsigned int Tex2dImpl::getHandle() const
{
    return static_cast<unsigned int>(mHandle);
}

void Tex2dImpl::genTexture()
{
    glGenTextures(1, &mHandle);
    glBindTexture(mTarget, mHandle);

    glTexParameteri(mTarget, GL_TEXTURE_WRAP_S, mWrapS);
    glTexParameteri(mTarget, GL_TEXTURE_WRAP_T, mWrapT);

    glTexParameteri(mTarget, GL_TEXTURE_MIN_FILTER, mMinFilter);
    glTexParameteri(mTarget, GL_TEXTURE_MAG_FILTER, mMagFilter);
}

void Tex2dImpl::init(unsigned char* str, long bufferSize)
{
    int width, height, comp;
    unsigned char* imageData = stbi_load_from_memory(str, static_cast<int>(bufferSize),
        &width, &height, &comp, (mPixelFormat == GL_RGB) ? STBI_rgb : STBI_rgb_alpha);

   if (imageData && width && height)
   {
        glTexStorage2D(mTarget, 1, mInternalFormat, width, height);
        glTexSubImage2D(mTarget, 0, 0, 0, width, height, mPixelFormat, GL_UNSIGNED_BYTE, imageData);

        glGenerateMipmap(mTarget);
   }
   else
   {
       std::string fileError = "Error texture data\n";
       throw new std::runtime_error(fileError.c_str());
   }

   stbi_image_free(imageData);
}

/**
 * @brief Tex2dImpl::init
 * @param width
 * @param height
 */
void Tex2dImpl::init(const unsigned int width, const unsigned int height)
{
    glTexStorage2D(mTarget, 1, mInternalFormat, width, height);
}
