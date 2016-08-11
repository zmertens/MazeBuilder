#ifndef WAVEFRONTOBJECTLOADER_HPP
#define WAVEFRONTOBJECTLOADER_HPP


#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "SdlManager.hpp"
#include "Vertex.hpp"

class WavefrontObjectLoader
{
public:
    WavefrontObjectLoader(const SdlManager& sdlManager);
    void parseFile(const std::string& filename, std::vector<Vertex>& vertices, std::vector<GLushort>& indices) const;
    void parseFile(const std::string& filename, std::vector<Vertex>& vertices, std::vector<GLushort>& indices,
        const std::string& verticesFile, const std::string& indicesFile) const;
private:
    const SdlManager& cSdlManager;
private:
    void trimString(std::string& str) const;
    void generateTangents(std::vector<Vertex>& vertices, std::vector<GLushort>& indices) const;
    void printVerticesAndIndicesToFile(const std::vector<Vertex>& vertices, const std::vector<GLushort>& indices,
        const std::string& verticesFile, const std::string& indicesFile) const;
};

#endif // WAVEFRONTOBJECTLOADER_HPP
