#include "MeshFactory.hpp"

#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "IndexedMeshImpl.hpp"

/**
 * @brief MeshFactory::ProduceMesh
 * @param type
 * @return
 */
IMesh::Ptr MeshFactory::ProduceMesh(const Types& type)
{
    using glm::vec3;
    using glm::vec2;

    switch (type)
    {
        case Types::TRIANGLE:
        {
            const std::vector<Vertex> vertices = {
                Vertex(vec3(1.0f, 0.0f, 1.0f), vec2(1.0f, 0.0f), vec3(-0.0f, 1.0f, 0.0f), vec3(-0.447657f, 0.0f, 0.894206f)),
                Vertex(vec3(1.0f, 0.0f, -1.0f), vec2(0.0f, 0.400297f), vec3(-0.0f, 1.0f, 0.0f), vec3(-0.447657f, 0.0f, 0.894206f)),
                Vertex(vec3(-1.0f, -0.0f, -0.00124f), vec2(1.0f, 1.0f), vec3(-0.0f, 1.0f, 0.0f), vec3(-0.447657f, 0.0f, 0.894206f))
            };
            const std::vector<GLushort> indices = {
                    0, 1, 2
            };

            IMesh::Ptr triangle (new IndexedMeshImpl(vertices, indices));
            return triangle;
        }
        case Types::PLANE:
        {
            const std::vector<Vertex> vertices = {
                    Vertex(vec3(-1.0f, -1.0f, 0.0f), vec2(0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),
                    Vertex(vec3(1.0f, -1.0f, 0.0f), vec2(1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),
                    Vertex(vec3(-1.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),
                    Vertex(vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f))
            };
            const std::vector<GLushort> indices = {
                    0, 1, 2,
                    1, 3, 2
            };

            IMesh::Ptr plane (new IndexedMeshImpl(vertices, indices));
            return plane;
        }
        case Types::CUBE:
        {
            const std::vector<Vertex> vertices = {
                // cube - front
                Vertex(glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                Vertex(glm::vec3(1.0f, -1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                Vertex(glm::vec3(1.0f,  1.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                Vertex(glm::vec3(-1.0f,  1.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                // cube - right
                Vertex(glm::vec3(1.0f, -1.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                Vertex(glm::vec3(1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(1.0, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                Vertex(glm::vec3(1.0f, 1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(1.0, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                Vertex(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(1.0, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                // cube - back
                Vertex(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                Vertex(glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                Vertex(glm::vec3(1.0f,  1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                Vertex(glm::vec3(1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                // cube - left
                Vertex(glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                Vertex(glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                Vertex(glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                Vertex(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                // cube - bottom
                Vertex(glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                Vertex(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                Vertex(glm::vec3(1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                Vertex(glm::vec3(1.0f, -1.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // cube - top
                Vertex(glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
                Vertex(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
                Vertex(glm::vec3(1.0f, 1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
                Vertex(glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f))
            };

            const std::vector<GLushort> indices = {
                // front
                0, 1, 2,
                0, 2, 3,
                // right
                4, 5, 6,
                4, 6, 7,
                // back
                8, 9, 10,
                8, 10, 11,
                // left
                12, 13, 14,
                12, 14, 15,
                // bottom
                16, 17, 18,
                16, 18, 19,
                // right
                20, 21, 22,
                20, 22, 23
            };

            IMesh::Ptr cube (new IndexedMeshImpl(vertices, indices));
            return cube;
        } // cube
    } // switch
} // ProduceMesh

/**
 * @brief MeshFactory::ProduceMesh
 * @param type
 * @param sdl
 * @param filename
 * @return
 */
IMesh::Ptr MeshFactory::ProduceMesh(const Types& type,
    const SdlManager& sdl,
    const std::string& filename)
{
//    WavefrontObjectLoader loader (sdl);
//    std::vector<Vertex> vertices;
//    std::vector<GLushort> indices;
//    loader.parseFile(filename, vertices, indices);

//    switch (type)
//    {
//        case Types::MONKEY:
//        case Types::SPHERE:
//        {
//            Mesh::Ptr mesh (new Mesh(vertices, indices));
//            return mesh;
//        }
//        case Types::ROCK:
//        {
//            std::vector<glm::mat4> instanceTransforms;
//            GLuint totalInstances = 200u;
//            GLfloat radius = 10.0f; // radius of imaginary circle
//            GLfloat offset = 1.95f; // displacement of instance objects
//            for (GLuint i = 0; i != totalInstances; ++i)
//            {
//                glm::mat4 model;
//                // 1. Translation: displace along circle with 'radius' in range [-offset, offset]
//                GLfloat angle = static_cast<GLfloat>(i) / static_cast<GLfloat>(totalInstances) * 360.0f;
//                GLfloat displacement = Utils::getRandomFloat(-offset, offset);
//                GLfloat x = glm::sin(angle) * radius + displacement;
//                displacement = Utils::getRandomFloat(-offset, offset);
//                GLfloat y = displacement * 0.4f; // y value has smaller displacement
//                displacement = Utils::getRandomFloat(-offset, offset);
//                GLfloat z = glm::cos(angle) * radius + displacement;
//                model = glm::translate(model, glm::vec3(x, y, z));
//                // 2. Rotation: add random rotation around a rotation axis vector
//                GLfloat rotAngle = Utils::getRandomFloat(0.0f, 360.0f);
//                model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));
//                // 3. Scale: Scale between 0.05 and 0.25f
//                GLfloat scale = Utils::getRandomFloat(0.05f, 0.35f);
//                model = glm::scale(model, glm::vec3(scale));
//                // 4. Now add to list of matrices
//                instanceTransforms.push_back(model);
//            }

//            Mesh::Ptr mesh (new Mesh(vertices, indices, instanceTransforms));
//            return mesh;
//        }
//    }
}
