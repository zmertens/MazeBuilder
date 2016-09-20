#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "IDrawable.hpp"

class Skybox : public IDrawable
{
public:
    explicit Skybox(const Draw::Config& config);
    virtual void update(float dt, double timeSinceInit) override;
    virtual void draw(const SdlWindow& sdlManager,
        ResourceManager& rm,
        const Camera& camera,
        const IMesh::Draw type = IMesh::Draw::TRIANGLES) const override;
    virtual void cleanUp() override;

private:
    Draw::Config mConfig;
};

#endif // SKYBOX_HPP
