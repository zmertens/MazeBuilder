#ifndef IMESH
#define IMESH

#include <memory>

/**
 * @brief The IMesh class
 */
class IMesh
{
public:
    typedef std::unique_ptr<IMesh> Ptr;

    enum class Draw {
        TRIANGLES,
        TRIANGLE_STRIP,
        LINES,
        POINTS
    };
public:
    virtual void cleanUp() = 0;
    virtual void update(float dt, double timeSinceInit) = 0;
    virtual void draw(IMesh::Draw type = IMesh::Draw::TRIANGLES,
        const unsigned int count = 4) const = 0;

protected:
    virtual void genBuffers() = 0;
    virtual void initMesh() = 0;

};

#endif // IMESH

