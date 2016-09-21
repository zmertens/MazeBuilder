#include "MeshImpl.hpp"

/**
 * @brief MeshImpl::MeshImpl
 */
MeshImpl::MeshImpl()
: mVaoHandle(0)
{
    genBuffers();
    initMesh();
}

/**
 * @brief MeshImpl::update
 * @param dt
 * @param timeSinceInit
 */
void MeshImpl::update(float dt, double timeSinceInit)
{

}

/**
 * @brief MeshImpl::draw
 * @param type = IMesh::Draw::TRIANGLES
 * @param count = 4
 */
void MeshImpl::draw(IMesh::Draw type, const unsigned int count) const
{
    glBindVertexArray(mVaoHandle);
    glDrawArrays(getGlType(type), 0, count);
#if defined(BLOWTORCH_DEBUG_MODE)
    glBindVertexArray(0);
#endif // defined
}

/**
 * @brief MeshImpl::cleanUp
 */
void MeshImpl::cleanUp()
{
    glDeleteVertexArrays(1, &mVaoHandle);
}

/**
 * @brief MeshImpl::genBuffers
 */
void MeshImpl::genBuffers()
{
    glGenVertexArrays(1, &mVaoHandle);
}

/**
 * @brief MeshImpl::initMesh
 */
void MeshImpl::initMesh()
{

}

/**
 * @brief MeshImpl::getGlType
 * @param type
 * @return
 */
GLenum MeshImpl::getGlType(IMesh::Draw type) const
{
    switch (type)
    {
        case IMesh::Draw::TRIANGLES: return GL_TRIANGLES;
        case IMesh::Draw::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        case IMesh::Draw::LINES: return GL_LINES;
        case IMesh::Draw::POINTS: return GL_POINTS;
    }
}

