#include "WavefrontObjectLoader.hpp"

#include <cstdlib> // atoi
#include <fstream>
#include <sstream> // istringstream

WavefrontObjectLoader::WavefrontObjectLoader(const SdlManager& sdlManager)
: cSdlManager(sdlManager)
{
    //ctor
}

void WavefrontObjectLoader::trimString(std::string& str) const
{
    const char* whiteSpace = " \t\n\r";
    std::size_t location;
    location = str.find_first_not_of(whiteSpace);
    str.erase(0, location);
    location = str.find_last_not_of(whiteSpace);
    str.erase(location + 1);
}

void WavefrontObjectLoader::generateTangents(std::vector<Vertex>& vertices, std::vector<GLushort>& indices) const
{
    std::vector<glm::vec3> tan1Accum;
//    std::vector<glm::vec3> tan2Accum;

    for(unsigned int i = 0; i < vertices.size(); i++)
    {
        tan1Accum.push_back(glm::vec3(0.0f));
//        tan2Accum.push_back(glm::vec3(0.0f));
    }

    // Compute the tangent vector
    for(unsigned int i = 0; i < indices.size(); i += 3)
    {
        const glm::vec3& p1 = vertices.at(indices[i]).position;
        const glm::vec3& p2 = vertices.at(indices[i+1]).position;
        const glm::vec3& p3 = vertices.at(indices[i+2]).position;

        const glm::vec2& tc1 = vertices.at(indices[i]).texCoord;
        const glm::vec2& tc2 = vertices.at(indices[i+1]).texCoord;
        const glm::vec2& tc3 = vertices.at(indices[i+2]).texCoord;

        glm::vec3 q1 = p2 - p1;
        glm::vec3 q2 = p3 - p1;
        float s1 = tc2.x - tc1.x, s2 = tc3.x - tc1.x;
        float t1 = tc2.y - tc1.y, t2 = tc3.y - tc1.y;
        float r = 1.0f / (s1 * t2 - s2 * t1);
        glm::vec3 tan1 ( (t2*q1.x - t1*q2.x) * r, (t2*q1.y - t1*q2.y) * r, (t2*q1.z - t1*q2.z) * r);
//        glm::vec3 tan2 ( (s1*q2.x - s2*q1.x) * r, (s1*q2.y - s2*q1.y) * r, (s1*q2.z - s2*q1.z) * r);
        tan1Accum[indices[i]] += tan1;
        tan1Accum[indices[i+1]] += tan1;
        tan1Accum[indices[i+2]] += tan1;
//        tan2Accum[indices[i]] += tan2;
//        tan2Accum[indices[i+1]] += tan2;
//        tan2Accum[indices[i+2]] += tan2;
    }

    for(unsigned int i = 0; i < vertices.size(); ++i)
    {
        const glm::vec3& n = vertices.at(i).normal;
        glm::vec3 &t1 = tan1Accum[i];
//        glm::vec3 &t2 = tan2Accum[i];

        // Gram-Schmidt orthogonalize
        vertices.at(i).tangent = glm::vec3(glm::normalize( t1 - (glm::dot(n,t1) * n) ));//, 0.0f);
        // Store handedness in w
        //tangents[i].w = (glm::dot( glm::cross(n,t1), t2 ) < 0.0f) ? -1.0f : 1.0f;
    }
    tan1Accum.clear();
//    tan2Accum.clear();
}

void WavefrontObjectLoader::printVerticesAndIndicesToFile(const std::vector<Vertex>& vertices,
    const std::vector<GLushort>& indices,
    const std::string& verticesFile, const std::string& indicesFile) const
{
    std::ofstream verticesOutFile;
    verticesOutFile.open(verticesFile);

    for (unsigned int iPtr = 0; iPtr != vertices.size(); iPtr++)
    {
        verticesOutFile << vertices.at(iPtr).position.x << "f " << vertices.at(iPtr).position.y << "f " << vertices.at(iPtr).position.z << "f "
        << vertices.at(iPtr).texCoord.x << "f " << vertices.at(iPtr).texCoord.y << "f "
        << vertices.at(iPtr).normal.x  << "f " << vertices.at(iPtr).normal.y << "f " << vertices.at(iPtr).normal.z << "f ";
    }

    verticesOutFile.close();

    std::ofstream indicesOutFile;
    indicesOutFile.open(indicesFile);

    for (unsigned int iPtr = 0; iPtr + 2 < indices.size(); iPtr += 3)
    {
        indicesOutFile << indices.at(iPtr) << " " << indices.at(iPtr + 1) << " " << indices.at(iPtr + 2) << " ";
    }

    indicesOutFile.close();
}

/**
 * Parses a basic Wavefront object file.
 * It is assumed that the vertices are triangulated and the faces contain 3 vertices.
 * @brief WavefrontObjectLoader::parseFile
 * @param filename
 * @param vertices
 * @param indices
 */
void WavefrontObjectLoader::parseFile(const std::string& filename, std::vector<Vertex>& vertices,
    std::vector<GLushort>& indices) const
{
    std::string parsedFile = cSdlManager.buildStringFromFile(filename);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;

    std::string line;
    std::istringstream inputStream (parsedFile);

    bool firstFaceFound = true;

    while (std::getline(inputStream, line))
    {
        trimString(line);

        std::istringstream lineStream (line);
        std::string token;
        lineStream >> token;

        if (token.at(0) == '#' || token.at(0) == 'u' || token.at(0) == 'o' || token.at(0) == 'g' || token.at(0) == 's')
        {
            // ignore comments, groupings, and object names
        }
        else if (token == "v")
        {
            float x, y, z;
            lineStream >> x >> y >> z;
            positions.push_back(glm::vec3(x, y, z));
        }
        else if (token == "vt")
        {
            // Blender draws from the bottom-left up (u, 1.0f - v)
            float u, v;
            lineStream >> u >> v;
            texCoords.push_back(glm::vec2(u, v));
        }
        else if (token == "vn")
        {
            float x, y, z;
            lineStream >> x >> y >> z;
            normals.push_back(glm::vec3(x, y, z));
        }
        else if (token == "f")
        {
            if (firstFaceFound == true)
            {
                vertices.reserve(positions.size());
                vertices.resize(positions.size());
                firstFaceFound = false;
            }

            // each face line will contain 3 unique vertex data
            while (lineStream.good())
            {
                std::string vertString;
                lineStream >> vertString;
                unsigned int positionIndex = 0;
                unsigned int normalIndex = 0;
                unsigned int texCoordIndex = 0;

                std::size_t slash1 = vertString.find("/");
                positionIndex = atoi(vertString.substr(0, slash1).c_str()) - 1;
                // verify that this is NOT case: v
                if (slash1 != std::string::npos)
                {
                    std::size_t slash2 = vertString.find("/", slash1 + 1);

                    // case: v//n
                    if (slash2 != std::string::npos && slash2 == slash1 + 1)
                    {
                        normalIndex = atoi(vertString.substr(slash2 + 1, vertString.length()).c_str()) - 1;

                        vertices.at(positionIndex) = Vertex(positions.at(positionIndex), glm::vec2(0.0f), normals.at(normalIndex));
                    }
                    // case: v/u/n
                    else if (slash2 != std::string::npos && slash2 > slash1 + 1)
                    {
                        texCoordIndex = atoi(vertString.substr(slash1 + 1, slash2).c_str()) - 1;
                        normalIndex = atoi(vertString.substr(slash2 + 1, vertString.length()).c_str()) - 1;

                        vertices.at(positionIndex) = Vertex(positions.at(positionIndex), texCoords.at(texCoordIndex), normals.at(normalIndex));

                    }
                    // case: v/u/
                    else
                    {
                        texCoordIndex = atoi(vertString.substr(slash1 + 1, slash2).c_str()) - 1;

                        vertices.at(positionIndex) = Vertex(positions.at(positionIndex), texCoords.at(texCoordIndex));
                    }
                }
                else
                {
                    vertices.at(positionIndex) = Vertex(positions.at(positionIndex));
                }

                indices.push_back(positionIndex);

            } // while(lineStream.good() ...)
        } // else if (token == "f")
    } // while (!inputStream.eof())

    generateTangents(vertices, indices);

#if defined(APP_DEBUG)
        std::stringstream ss;
        ss << "Loaded mesh from: " << filename << "\n";
        ss << positions.size() << " points" << "\n";
        ss << indices.size() / 3 << " triangles (or faces)" << "\n";
        ss << normals.size() << " normals" << "\n";
        ss << texCoords.size() << " texture coordinates" << "\n";
        SDL_Log(ss.str().c_str());
#endif // defined
} // parseFile(... )

/**
 * Parses a basic Wavefront object file.
 * It is assumed that the vertices are triangulated and the faces contain 3 vertices.
 * @brief WavefrontObjectLoader::parseFile
 * @param filename
 * @param vertices
 * @param indices
 * @param verticesFile
 * @param indicesFile
 */
void WavefrontObjectLoader::parseFile(const std::string& filename, std::vector<Vertex>& vertices,
    std::vector<GLushort>& indices,
    const std::string& verticesFile, const std::string& indicesFile) const
{
    std::string parsedFile = cSdlManager.buildStringFromFile(filename);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;

    std::string line;
    std::istringstream inputStream (parsedFile);

    bool firstFaceFound = true;

    while (std::getline(inputStream, line))
    {
        trimString(line);

        std::istringstream lineStream (line);
        std::string token;
        lineStream >> token;

        if (token.at(0) == '#' || token.at(0) == 'u' || token.at(0) == 'o' || token.at(0) == 'g' || token.at(0) == 's')
        {
            // ignore comments, groupings, and object names
        }
        else if (token == "v")
        {
            float x, y, z;
            lineStream >> x >> y >> z;
            positions.push_back(glm::vec3(x, y, z));
        }
        else if (token == "vt")
        {
            // Blender draws from the bottom-left up (u, 1.0f - v)
            float u, v;
            lineStream >> u >> v;
            texCoords.push_back(glm::vec2(u, v));
        }
        else if (token == "vn")
        {
            float x, y, z;
            lineStream >> x >> y >> z;
            normals.push_back(glm::vec3(x, y, z));
        }
        else if (token == "f")
        {
            if (firstFaceFound == true)
            {
                vertices.reserve(positions.size());
                vertices.resize(positions.size());
                firstFaceFound = false;
            }

            // each face line will contain 3 unique vertex data
            while (lineStream.good())
            {
                std::string vertString;
                lineStream >> vertString;
                unsigned int positionIndex = 0;
                unsigned int normalIndex = 0;
                unsigned int texCoordIndex = 0;

                std::size_t slash1 = vertString.find("/");
                positionIndex = atoi(vertString.substr(0, slash1).c_str()) - 1;
                // verify that this is NOT case: v
                if (slash1 != std::string::npos)
                {
                    std::size_t slash2 = vertString.find("/", slash1 + 1);

                    // case: v//n
                    if (slash2 != std::string::npos && slash2 == slash1 + 1)
                    {
                        normalIndex = atoi(vertString.substr(slash2 + 1, vertString.length()).c_str()) - 1;

                        vertices.at(positionIndex) = Vertex(positions.at(positionIndex), glm::vec2(0.0f), normals.at(normalIndex));
                    }
                    // case: v/u/n
                    else if (slash2 != std::string::npos && slash2 > slash1 + 1)
                    {
                        texCoordIndex = atoi(vertString.substr(slash1 + 1, slash2).c_str()) - 1;
                        normalIndex = atoi(vertString.substr(slash2 + 1, vertString.length()).c_str()) - 1;

                        vertices.at(positionIndex) = Vertex(positions.at(positionIndex), texCoords.at(texCoordIndex), normals.at(normalIndex));

                    }
                    // case: v/u/
                    else
                    {
                        texCoordIndex = atoi(vertString.substr(slash1 + 1, slash2).c_str()) - 1;

                        vertices.at(positionIndex) = Vertex(positions.at(positionIndex), texCoords.at(texCoordIndex));
                    }
                }
                else
                {
                    vertices.at(positionIndex) = Vertex(positions.at(positionIndex));
                }

                indices.push_back(positionIndex);

            } // while(lineStream.good() ...)
        } // else if (token == "f")
    } // while (!inputStream.eof())

    generateTangents(vertices, indices);

#if defined(APP_DEBUG)
        std::stringstream ss;
        ss << "Loaded mesh from: " << filename << "\n";
        ss << positions.size() << " points" << "\n";
        ss << indices.size() / 3 << " triangles (or faces)" << "\n";
        ss << normals.size() << " normals" << "\n";
        ss << texCoords.size() << " texture coordinates" << "\n";
        SDL_Log(ss.str().c_str());
#endif // defined

    printVerticesAndIndicesToFile(vertices, indices, verticesFile, indicesFile);
} // parseFile(... )


