#include "TexSkyboxImpl.hpp"

#if defined(GAME_DEBUG_MODE)
#include "GlUtils.hpp"
#endif // defined

#include "../Utils.hpp"

#include "stb_image.h"

GLuint TexSkyboxImpl::sCubeMapIndex = 0;

/**
 * @brief TexSkyboxImpl::TexSkyboxImpl
 * @param sdlManager
 * @param fileNames
 * @param channel
 */
TexSkyboxImpl::TexSkyboxImpl(const SdlWindow& sdlManager,
    const std::vector<std::string>& fileNames,
    unsigned int channel)
: mChannel(channel)
, mTarget(GL_TEXTURE_CUBE_MAP)
, mInternalFormat(GL_RGB8)
, mPixelFormat(GL_RGB)
, mWrapS(GL_CLAMP_TO_EDGE)
, mWrapT(GL_CLAMP_TO_EDGE)
, mMinFilter(GL_LINEAR)
, mMagFilter(GL_LINEAR)
, mHandle(0)
{
    genTexture();

    for (sCubeMapIndex = 0; sCubeMapIndex != fileNames.size(); ++sCubeMapIndex)
    {
        long bufferSize;
        unsigned char* str = sdlManager.buildBufferFromFile(fileNames.at(sCubeMapIndex), bufferSize);

        init(str, bufferSize);
    }
}

/**
 * @brief TexSkyboxImpl::cleanUp
 */
void TexSkyboxImpl::cleanUp()
{
    glDeleteTextures(1, &mHandle);
}

/**
 * @brief TexSkyboxImpl::bind
 */
void TexSkyboxImpl::bind() const
{
    // glBindTextureUnit(mChannel, mHandle);
    glActiveTexture(GL_TEXTURE0 + mChannel);
    glBindTexture(mTarget, mHandle);
}

/**
 * @brief TexSkyboxImpl::release
 */
void TexSkyboxImpl::release() const
{
    glBindTexture(mTarget, 0);
}

/**
 * @brief TexSkyboxImpl::getHandle
 * @return
 */
unsigned int TexSkyboxImpl::getHandle() const
{
    return static_cast<unsigned int>(mHandle);
}

/**
 * @brief TexSkyboxImpl::genTexture
 */
void TexSkyboxImpl::genTexture()
{
    glGenTextures(1, &mHandle);
    glBindTexture(mTarget, mHandle);

    glTexParameteri(mTarget, GL_TEXTURE_WRAP_S, mWrapS);
    glTexParameteri(mTarget, GL_TEXTURE_WRAP_T, mWrapT);

    glTexParameteri(mTarget, GL_TEXTURE_MIN_FILTER, mMinFilter);
    glTexParameteri(mTarget, GL_TEXTURE_MAG_FILTER, mMagFilter);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

/**
 * @brief TexSkyboxImpl::init
 * @param fileNames
 */
void TexSkyboxImpl::init(unsigned char* str, long bufferSize)
{
    int width, height, comp;
    unsigned char* imageData = stbi_load_from_memory(str, static_cast<int>(bufferSize),
        &width, &height, &comp, (mPixelFormat == GL_RGB) ? STBI_rgb : STBI_rgb_alpha);

    if (imageData && width && height)
    {
        // Requires OpenGL 4.2
        // // glTexStorage2D only needs to be set once since every cubemap texture has same dimensions
        // if (sCubeMapIndex == 0)
        //     glTexStorage2D(mTarget, 1, mInternalFormat, width, height);

        // glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + sCubeMapIndex, 0, 0, 0, width, height,
        //     mPixelFormat, GL_UNSIGNED_BYTE, imageData);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + sCubeMapIndex, 0, mInternalFormat, width, height, 0, mPixelFormat, GL_UNSIGNED_BYTE, imageData);
    }
    else
    {
       std::string fileError = "Error loading data from texture file:\n";
       throw new std::runtime_error(fileError.c_str());
    }

#if defined(GAME_DEBUG_MODE)
    std::string fileComp = "Texture generated from " + Utils::toString(str) + ", width = "
       + Utils::toString(width) + ", height = "
       + Utils::toString(height) + ", comp = " + Utils::toString(comp) + "\n";
    SDL_Log(fileComp.c_str());
#endif // defined

    stbi_image_free(imageData);
} // init

/**
 * @brief TexSkyboxImpl::init
 * @param width
 * @param height
 */
void TexSkyboxImpl::init(const unsigned int width, const unsigned int height)
{
    // no impl
}


