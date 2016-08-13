#ifndef INDEXEDMESHIMPL_HPP
#define INDEXEDMESHIMPL_HPP

#include <vector>

#include "IMesh.hpp"
#include "../SdlManager.hpp"
#include "../Vertex.hpp"

class IndexedMeshImpl : public IMesh
{
public:
    explicit IndexedMeshImpl(const std::vector<Vertex>& vertices,
        const std::vector<GLushort>& indices);

    virtual void update(float dt, double timeSinceInit) override;
    virtual void draw(IMesh::Draw type = IMesh::Draw::TRIANGLES,
        const unsigned int count = 4) const override;

protected:
    virtual void cleanUp() override;
    virtual void genBuffers() override;
    virtual void initMesh() override;
private:
    const std::vector<Vertex>& cVertices;
    const std::vector<GLushort>& cIndices;
    GLuint mVaoHandle;
    GLuint mVboHandle;
    GLuint mIboHandle;
    GLuint mNumVertices;
    GLuint mNumIndices;
private:
    GLenum getGlType(IMesh::Draw type) const;
};

#endif // INDEXEDMESHIMPL_HPP
