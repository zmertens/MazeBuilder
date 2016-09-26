#include "IndexedMeshImpl.hpp"

#include <vector>

#include "../Vertex.hpp"

/**
 * @brief IndexedMeshImpl::IndexedMeshImpl
 * @param vertices
 * @param indices
 */
IndexedMeshImpl::IndexedMeshImpl(const std::vector<Vertex>& vertices,
    const std::vector<GLushort>& indices)
: cVertices(vertices)
, cIndices(indices)
, mNumVertices(vertices.size())
, mNumIndices(indices.size())
{
    genBuffers();
    initMesh();
}

/**
 * @brief IndexedMeshImpl::update
 * @param dt
 * @param timeSinceInit
 */
void IndexedMeshImpl::update(float dt, double timeSinceInit)
{

}

/**
 * @brief IndexedMeshImpl::draw
 * @param type = IMesh::Draw::TRIANGLES
 * @param count = 4
 */
void IndexedMeshImpl::draw(IMesh::Draw type,
    const unsigned int count) const
{
    glBindVertexArray(mVaoHandle);
    glDrawElements(getGlType(type), mNumIndices, GL_UNSIGNED_SHORT, 0);
#if defined(GAME_DEBUG_MODE)
    glBindVertexArray(0);
#endif // defined
}

/**
 * @brief IndexedMeshImpl::cleanUp
 */
void IndexedMeshImpl::cleanUp()
{
    glDeleteVertexArrays(1, &mVaoHandle);
    glDeleteBuffers(1, &mVboHandle);
    glDeleteBuffers(1, &mIboHandle);
}

/**
 * @brief IndexedMeshImpl::genBuffers
 */
void IndexedMeshImpl::genBuffers()
{
    glGenVertexArrays(1, &mVaoHandle);
    glGenBuffers(1, &mVboHandle);
    glGenBuffers(1, &mIboHandle);
}

/**
 * @brief IndexedMeshImpl::initMesh
 */
void IndexedMeshImpl::initMesh()
{
    glBindVertexArray(mVaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mVboHandle);
    glBufferData(GL_ARRAY_BUFFER, mNumVertices * sizeof(Vertex), cVertices.data(), GL_STATIC_DRAW);
    // vertex position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, position)));
    // vertex texture coordinate
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, texCoord)));
    // vertex normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, normal)));
    // vertex tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, tangent)));
    // indices data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIboHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mNumIndices * sizeof(GLushort), cIndices.data(), GL_STATIC_DRAW);

#if defined(GAME_DEBUG_MODE)
    glBindVertexArray(0);
#endif // defined
}

/**
 * @brief IndexedMeshImpl::getGlType
 * @param type
 * @return
 */
GLenum IndexedMeshImpl::getGlType(IMesh::Draw type) const
{
    switch (type)
    {
        case IMesh::Draw::TRIANGLES: return GL_TRIANGLES;
        case IMesh::Draw::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        case IMesh::Draw::LINES: return GL_LINES;
        case IMesh::Draw::POINTS: return GL_POINTS;
    }
}
