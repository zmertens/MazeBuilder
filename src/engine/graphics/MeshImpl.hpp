#ifndef MESHIMPL_HPP
#define MESHIMPL_HPP

#include "IMesh.hpp"
#include "../SdlManager.hpp"

class MeshImpl : public IMesh
{
public:
    explicit MeshImpl();
    virtual void update(float dt, double timeSinceInit) override;
    virtual void draw(IMesh::Draw type = IMesh::Draw::TRIANGLES,
        const unsigned int count = 4) const override;

protected:
    virtual void cleanUp() override;
    virtual void genBuffers() override;
    virtual void initMesh() override;
private:
    GLuint mVaoHandle;
private:
    GLenum getGlType(IMesh::Draw type) const;
};

#endif // MESHIMPL_HPP
