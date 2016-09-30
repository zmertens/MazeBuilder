#ifndef POSTPROCESSORIMPL_HPP
#define POSTPROCESSORIMPL_HPP

#include "IFramebuffer.hpp"

#include "IDrawable.hpp"

#include "Tex2dImpl.hpp" // to declare gl stuff

namespace Effects
{
// layout matches effects shader
enum class Type : int {
    None = 0,
    Grayscale = 1,
    Inversion = 2,
    Edge = 3,
    Blur = 4,
    Sharpen = 5,
};
} // namespace

class ResourceManager;

class PostProcessorImpl : public IFramebuffer
{
public:
    using Ptr = std::unique_ptr<PostProcessorImpl>;
public:
    explicit PostProcessorImpl(const ResourceManager& resources,
        const Draw::Config& config,
        const unsigned int width, const unsigned int height);

    virtual void cleanUp() override;
    virtual void bind() const override;
    virtual void release() const override;
    virtual unsigned int getHandle() const override;

    void activateEffect(Effects::Type type);

protected:
    virtual void genFrameBuffer() override;
    virtual void init(const unsigned int width, const unsigned int height) override;

private:
    const ResourceManager& mResources;
    Draw::Config mConfig;
    Effects::Type mEffect;
    GLuint mFboHandle;
    GLuint mRboHandle;
};

#endif // POSTPROCESSORIMPL_HPP
