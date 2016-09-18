#include "TexPerlinImpl.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include "stb_image.h"

/**
 * @brief TexPerlinImpl::TexPerlinImpl
 * @param baseFrequency
 * @param persistence
 * @param width
 * @param height
 * @param periodic
 * @param channel = 0
 */
TexPerlinImpl::TexPerlinImpl(const float baseFrequency,
    const float persistence,
    const int width, const int height,
    bool periodic, unsigned int channel)
: mChannel(channel)
, mTarget(GL_TEXTURE_2D)
, mInternalFormat(GL_RGBA8)
, mPixelFormat(GL_RGBA)
, mWrapS(GL_CLAMP_TO_EDGE)
, mWrapT(GL_CLAMP_TO_EDGE)
, mMinFilter(GL_LINEAR_MIPMAP_LINEAR)
, mMagFilter(GL_LINEAR)
, mHandle(0)
{
    genTexture();

    mTexData = new GLubyte[width * height * 4];

    GLdouble xFactor = 1.0f / (width - 1);
    GLdouble yFactor = 1.0f / (height - 1);

    for (int row = 0; row < height; row++)
    {
        for (int col = 0 ; col < width; col++)
        {
            GLfloat x = xFactor * col;
            GLfloat y = yFactor * row;
            GLfloat sum = 0.0f;
            GLfloat freq = baseFrequency;
            GLfloat persist = persistence;

            for (int oct = 0; oct < 4; oct++)
            {
                glm::vec2 p(x * freq, y * freq);

                GLfloat val = 0.0f;

                if (periodic)
                    val = glm::perlin(p, glm::vec2(freq)) * persist;
                else
                    val = glm::perlin(p) * persist;

                sum += val;

                GLfloat result = (sum + 1.0f) / 2.0f;

                // Clamp strictly between 0 and 1
                result = result > 1.0f ? 1.0f : result;
                result = result < 0.0f ? 0.0f : result;

                // Store in texture
                mTexData[((row * width + col) * 4) + oct] = static_cast<GLubyte>(result * 255.0f);
                freq *= 2.0f;
                persist *= persistence;
            }
        }
    }

    init(width, height);
}

void TexPerlinImpl::cleanUp()
{
    glDeleteTextures(1, &mHandle);
}

void TexPerlinImpl::bind() const
{
    glBindTextureUnit(mChannel, mHandle);
}

void TexPerlinImpl::release() const
{
    glBindTexture(mTarget, 0);
}

unsigned int TexPerlinImpl::getHandle() const
{
    return static_cast<unsigned int>(mHandle);
}

void TexPerlinImpl::genTexture()
{
    glGenTextures(1, &mHandle);
    glBindTexture(mTarget, mHandle);

    glTexParameteri(mTarget, GL_TEXTURE_WRAP_S, mWrapS);
    glTexParameteri(mTarget, GL_TEXTURE_WRAP_T, mWrapT);

    glTexParameteri(mTarget, GL_TEXTURE_MIN_FILTER, mMinFilter);
    glTexParameteri(mTarget, GL_TEXTURE_MAG_FILTER, mMagFilter);
}

void TexPerlinImpl::init(unsigned char* str, long bufferSize)
{

}

void TexPerlinImpl::init(const unsigned int width, const unsigned int height)
{
    glTexStorage2D(mTarget, 1, mInternalFormat, width, height);
    glTexSubImage2D(mTarget, 0, 0, 0, width, height, mPixelFormat, GL_UNSIGNED_BYTE, mTexData);

}
