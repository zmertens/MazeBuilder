#include "PostProcessorImpl.hpp"

#include "../ResourceManager.hpp"

#if defined(BLOWTORCH_DEBUG_MODE)
#include "GlUtils.hpp"
#endif // defined

PostProcessorImpl::PostProcessorImpl(const ResourceManager& resources,
    const Draw::Config& config,
    const unsigned int width, const unsigned int height)
: mResources(resources)
, mConfig(config)
, mEffect(Effects::Type::None)
, mFboHandle(0)
, mRboHandle(0)
, fullscreen(width, height, 1)
{
    genFrameBuffer();
    init(width, height);
}

void PostProcessorImpl::cleanUp()
{
    if (mFboHandle)
        glDeleteFramebuffers(1, &mFboHandle);
    if (mRboHandle)
        glDeleteRenderbuffers(1, &mRboHandle);
}

void PostProcessorImpl::bind() const
{
    fullscreen.bind();
    glBindFramebuffer(GL_FRAMEBUFFER, mFboHandle);
}

void PostProcessorImpl::release() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    static GLfloat time = 0.05f;
    if (time > 0.0f)
        time -= 1.f / 60.f;
    else
        time = 0.05f;

    glDisable(GL_DEPTH_TEST);

    // draw to the screen
    auto& shader = mResources.getShader(mConfig.shaderId);
    shader->bind();
    shader->setUniform("uEffect.type", static_cast<GLint>(mEffect));
    shader->setUniform("uTime", time);

    auto& mesh = mResources.getMesh(mConfig.meshId);
    mesh->draw(IMesh::Draw::TRIANGLE_STRIP);
}

unsigned int PostProcessorImpl::getHandle() const
{
    return mFboHandle;
}

void PostProcessorImpl::activateEffect(Effects::Type type)
{
    mEffect = type;
}

void PostProcessorImpl::genFrameBuffer()
{
    glGenFramebuffers(1, &mFboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, mFboHandle);

    glGenRenderbuffers(1, &mRboHandle);
}

void PostProcessorImpl::init(const unsigned int width, const unsigned int height)
{
    fullscreen.bind();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fullscreen.getHandle(), 0);

    glBindRenderbuffer(GL_RENDERBUFFER, mRboHandle);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRboHandle);

#if defined(BLOWTORCH_DEBUG_MODE)
    SDL_Log("\nPostProcessorImpl using Texture channel = %i\n", 1);
    GlUtils::CheckForOpenGLError(__FILE__, __LINE__);
#endif // defined

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw new std::runtime_error("FrameBuffer Error! Failed to initialize mFrameBuffer_Handle\n");

    // if (glCheckNamedFramebufferStatus(mFboHandle, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) // 4.5
    //     throw new std::runtime_error("FrameBuffer Error! Failed to initialize mFrameBuffer_Handle\n");

#if defined(BLOWTORCH_DEBUG_MODE)
    GlUtils::CheckForOpenGLError(__FILE__, __LINE__);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif // defined
}

