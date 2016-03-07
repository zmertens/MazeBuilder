#include "PostProcessorImpl.hpp"

#include "../ResourceManager.hpp"

// kind of a dirty hack for now...
#include "Tex2dImpl.hpp"

#if APP_DEBUG == 1
#include "GlUtils.hpp"
#endif

PostProcessorImpl::PostProcessorImpl(const ResourceManager& resources,
    const Entity::Config& config,
    const unsigned int width, const unsigned int height)
: mResources(resources)
, mConfig(config)
, mEffect(Effects::Type::NO_EFFECT)
, mFboHandle(0)
, mRboHandle(0)
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
    glGenRenderbuffers(1, &mRboHandle);
}

void PostProcessorImpl::init(const unsigned int width, const unsigned int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFboHandle);

    // @todo this is kind of hack because the #2 channel is not used by anything else,
    // if the texture isn't bound nothing is rendered to it ....
    // could use ResourceManager textures but in taht case,
    // cannot initialize PostProcessor before the ResourceManager
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(0, &mode);

    Tex2dImpl fullscreen (width, height, 2);
    fullscreen.bind();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fullscreen.getHandle(), 0);

    glBindRenderbuffer(GL_RENDERBUFFER, mRboHandle);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRboHandle);

    if (APP_DEBUG)
        GlUtils::CheckForOpenGLError(__FILE__, __LINE__);

    if (glCheckNamedFramebufferStatus(mFboHandle, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw new std::runtime_error("FrameBuffer Error! Failed to initialize mFrameBuffer_Handle\n");

    if (APP_DEBUG)
        GlUtils::CheckForOpenGLError(__FILE__, __LINE__);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

