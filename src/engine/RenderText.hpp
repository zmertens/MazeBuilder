#ifndef RENDERTEXT_HPP
#define RENDERTEXT_HPP

#include "graphics/IMesh.hpp"
#include "SdlWindow.hpp"

class ResourceManager;
class Text;

class RenderText : public IMesh
{
public:
    explicit RenderText();
    virtual void update(float dt, double timeSinceInit) override;
    virtual void draw(IMesh::Draw type = IMesh::Draw::TRIANGLES,
        const unsigned int count = 4) const override;
    void renderText(const ResourceManager& rm, const Text& text) const;

protected:
    virtual void cleanUp() override;
    virtual void genBuffers() override;
    virtual void initMesh() override;
private:
    GLuint mVaoHandle;
    GLuint mVboHandle;
private:
    GLenum getGlType(IMesh::Draw type) const;
};

#endif // RENDERTEXT_HPP
