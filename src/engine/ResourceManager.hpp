#ifndef RESOURCEMANAGER_HPP
#define RESOURCEMANAGER_HPP

#include <string>
#include <unordered_map>
#include <tuple>
#include <memory>

#include "graphics/Shader.hpp"
#include "graphics/ITexture.hpp"
#include "graphics/Material.hpp"
#include "graphics/IMesh.hpp"
#include "audio/Chunk.hpp"
#include "audio/Music.hpp"

namespace CachePos
{
const unsigned int Shader = 0;
const unsigned int Material = 1;
const unsigned int Texture = 2;
const unsigned int Offset0 = 3;
const unsigned int Offset1 = 4;
const unsigned int Offset2 = 5;
}

class ResourceManager final
{
public:
    explicit ResourceManager();

    void insert(const std::string& id, IMesh::Ptr mesh);
    void insert(const std::string& id, Shader::Ptr shader);
    void insert(const std::string& id, Material::Ptr material);
    void insert(const std::string& id, ITexture::Ptr texture);
    void insert(const std::string& id, Music::Ptr music);
    void insert(const std::string& id, Chunk::Ptr chunk);

    const ITexture::Ptr& getTexture(const std::string& id) const;
    const Material::Ptr& getMaterial(const std::string& id) const;
    const IMesh::Ptr& getMesh(const std::string& id) const;
    const Shader::Ptr& getShader(const std::string& id) const;
    const Music::Ptr& getMusic(const std::string& id) const;
    const Chunk::Ptr& getChunk(const std::string& id) const;

    void putInCache(const std::string& id, const unsigned int index);
    void putInCache(const glm::vec2& id, const unsigned int index);
    bool isInCache(const std::string& id, const unsigned int index) const;
    bool isInCache(const glm::vec2& id, const unsigned int index) const;
    void clearCache();

    std::string getAllLogs() const;
    std::string getShaderLogs() const;
    std::string getTextureLogs() const;
    std::string getMaterialLogs() const;
    std::string getMeshLogs() const;
    std::string getMusicLogs() const;
    std::string getChunkLogs() const;

    void cleanUp();

    const std::unordered_map<std::string, IMesh::Ptr>& getMeshes() const;

private:
    std::unordered_map<std::string, Shader::Ptr> mShaders;
    std::unordered_map<std::string, ITexture::Ptr> mTextures;
    std::unordered_map<std::string, IMesh::Ptr> mMeshes;
    std::unordered_map<std::string, Material::Ptr> mMaterials;
    std::unordered_map<std::string, Music::Ptr> mMusic;
    std::unordered_map<std::string, Chunk::Ptr> mChunks;

    std::tuple<std::string, std::string, std::string,
        glm::vec2, glm::vec2, glm::vec2> mResourceCache;
private:
    ResourceManager(const ResourceManager& other);
    ResourceManager& operator=(const ResourceManager& other);
};

#endif // RESOURCEMANAGER_HPP
