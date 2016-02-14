#include "ResourceManager.hpp"

#include "SdlManager.hpp"

/**
 * @brief ResourceManager::ResourceManager
 */
ResourceManager::ResourceManager()
{

}

/**
 * @brief ResourceManager::insert
 * @param id
 * @param mesh
 */
void ResourceManager::insert(const std::string& id, IMesh::Ptr mesh)
{
    auto itr = mMeshes.find(id);
    if (itr == mMeshes.end())
    {
        mMeshes.emplace(id, std::move(mesh));
    }
    else
    {
        std::string warning = id + " has already been inserted into the mesh map\n";
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, warning.c_str());
    }
}

/**
 * @brief ResourceManager::insert
 * @param id
 * @param shader
 */
void ResourceManager::insert(const std::string& id, Shader::Ptr shader)
{
    auto itr = mShaders.find(id);
    if (itr == mShaders.end())
    {
        mShaders.emplace(id, std::move(shader));
    }
    else
    {
        std::string warning = id + " has already been inserted into the shaders map\n";
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, warning.c_str());
    }
}

/**
 * @brief ResourceManager::insert
 * @param id
 * @param material
 */
void ResourceManager::insert(const std::string& id, Material::Ptr material)
{
    auto itr = mMaterials.find(id);
    if (itr == mMaterials.end())
    {
        mMaterials.emplace(id, std::move(material));
    }
    else
    {
        std::string warning = id + " has already been inserted into the materials map\n";
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, warning.c_str());
    }
}

/**
 * @brief ResourceManager::insert
 * @param id
 * @param texture
 */
void ResourceManager::insert(const std::string& id, ITexture::Ptr texture)
{
    auto itr = mTextures.find(id);
    if (itr == mTextures.end())
    {
        mTextures.emplace(id, std::move(texture));
    }
    else
    {
        std::string warning = id + " has already been inserted into the textures map\n";
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, warning.c_str());
    }
}

/**
 * @brief ResourceManager::insert
 * @param id
 * @param music
 */
void ResourceManager::insert(const std::string& id, Music::Ptr music)
{
    auto itr = mMusic.find(id);
    if (itr == mMusic.end())
    {
        mMusic.emplace(id, std::move(music));
    }
    else
    {
        std::string warning = id + " has already been inserted into the music map\n";
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, warning.c_str());
    }
}

/**
 * @brief ResourceManager::insert
 * @param id
 * @param chunk
 */
void ResourceManager::insert(const std::string& id, Chunk::Ptr chunk)
{
    auto itr = mChunks.find(id);
    if (itr == mChunks.end())
    {
        mChunks.emplace(id, std::move(chunk));
    }
    else
    {
        std::string warning = id + " has already been inserted into the chunks map\n";
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, warning.c_str());
    }
}

/**
 * @brief ResourceManager::getTexture
 * @param id
 * @return
 */
const ITexture::Ptr& ResourceManager::getTexture(const std::string& id) const
{
    return mTextures.at(id);
}

/**
 * @brief ResourceManager::getMaterial
 * @param id
 * @return
 */
const Material::Ptr& ResourceManager::getMaterial(const std::string& id) const
{
    return mMaterials.at(id);
}

/**
 * @brief ResourceManager::getMesh
 * @param id
 * @return
 */
const IMesh::Ptr& ResourceManager::getMesh(const std::string& id) const
{
    return mMeshes.at(id);
}

/**
 * @brief ResourceManager::getShader
 * @param id
 * @return
 */
const Shader::Ptr& ResourceManager::getShader(const std::string& id) const
{
    return mShaders.at(id);
}

/**
 * @brief ResourceManager::getMusic
 * @param id
 * @return
 */
const Music::Ptr& ResourceManager::getMusic(const std::string& id) const
{
    return mMusic.at(id);
}

/**
 * @brief ResourceManager::getChunk
 * @param id
 * @return
 */
const Chunk::Ptr& ResourceManager::getChunk(const std::string& id) const
{
    return mChunks.at(id);
}

/**
 * @brief ResourceManager::putInCache
 * @param id
 * @param index
 */
void ResourceManager::putInCache(const std::string& id, const unsigned int index)
{
    switch (index)
    {
        case CachePos::Shader: std::get<CachePos::Shader>(mResourceCache) = id; break;
        case CachePos::Material: std::get<CachePos::Material>(mResourceCache) = id; break;
        case CachePos::Texture: std::get<CachePos::Texture>(mResourceCache) = id; break;
    }
}

/**
 * @brief ResourceManager::putInCache
 * @param id
 * @param index
 */
void ResourceManager::putInCache(const glm::vec2& id, const unsigned int index)
{
    switch (index)
    {
        case CachePos::Offset0: std::get<CachePos::Offset0>(mResourceCache) = id; break;
        case CachePos::Offset1: std::get<CachePos::Offset1>(mResourceCache) = id; break;
        case CachePos::Offset2: std::get<CachePos::Offset2>(mResourceCache) = id; break;
    }
}

/**
 * @brief ResourceManager::isInCache
 * @param id
 * @param index
 * @return
 */
bool ResourceManager::isInCache(const std::string& id, const unsigned int index) const
{
    switch (index)
    {
        case CachePos::Shader: return std::get<CachePos::Shader>(mResourceCache) == id;
        case CachePos::Material: return std::get<CachePos::Material>(mResourceCache) == id;
        case CachePos::Texture: return std::get<CachePos::Texture>(mResourceCache) == id;
    }
}

/**
 * @brief ResourceManager::isInCache
 * @param id
 * @param index
 * @return
 */
bool ResourceManager::isInCache(const glm::vec2& id, const unsigned int index) const
{
    switch (index)
    {
        case CachePos::Offset0:
            return (std::get<CachePos::Offset0>(mResourceCache).x == id.x &&
                std::get<CachePos::Offset0>(mResourceCache).y == id.y);
        case CachePos::Offset1:
            return (std::get<CachePos::Offset1>(mResourceCache).x == id.x &&
                std::get<CachePos::Offset1>(mResourceCache).y == id.y);
        case CachePos::Offset2:
            return (std::get<CachePos::Offset2>(mResourceCache).x == id.x &&
                std::get<CachePos::Offset2>(mResourceCache).y == id.y);
    }
}


/**
 * Resets cache.
 * @brief ResourceManager::clearCache
 */
void ResourceManager::clearCache()
{
    std::get<CachePos::Shader>(mResourceCache) = "";
    std::get<CachePos::Material>(mResourceCache) = "";
    std::get<CachePos::Texture>(mResourceCache) = "";
    std::get<CachePos::Offset0>(mResourceCache) = glm::vec2(-1);
    std::get<CachePos::Offset1>(mResourceCache) = glm::vec2(-1);
    std::get<CachePos::Offset2>(mResourceCache) = glm::vec2(-1);
}

/**
 * @brief ResourceManager::getAllLogs
 * @return
 */
std::string ResourceManager::getAllLogs() const
{
    std::string ret = "\nPrinting all Resource Logs:\n";
    ret += getShaderLogs();
    ret += getTextureLogs();
    ret += getMaterialLogs();
    ret += getMeshLogs();
    ret += getMusicLogs();
    ret += getChunkLogs();
    return ret;
}

/**
 * @brief ResourceManager::getShaderLogs
 * @return
 */
std::string ResourceManager::getShaderLogs() const
{
    std::string ret = "Printing Shader Logs:\n";
    for (auto& itr : mShaders)
    {
        ret += "Shader id: " + itr.first + "\n";
        ret += itr.second->getGlslAttribs();
        ret += itr.second->getGlslUniforms();
    }
    return ret;
}

/**
 * @brief ResourceManager::getTextureLogs
 * @return
 */
std::string ResourceManager::getTextureLogs() const
{
    std::string ret = "Printing Texture Logs:\n";
    for (auto& itr : mTextures)
        ret += "Texture id: " + itr.first + "\n";
    return ret;
}

/**
 * @brief ResourceManager::getMaterialLogs
 * @return
 */
std::string ResourceManager::getMaterialLogs() const
{
    std::string ret = "Printing Material Logs:\n";
    for (auto& itr : mMaterials)
        ret += "Material id: " + itr.first + "\n";
    return ret;
}

/**
 * @brief ResourceManager::getMeshLogs
 * @return
 */
std::string ResourceManager::getMeshLogs() const
{
    std::string ret = "Printing Mesh Logs:\n";
    for (auto& itr : mMeshes)
        ret += "Mesh id: " + itr.first + "\n";
    return ret;
}

/**
 * @brief ResourceManager::getMusicLogs
 * @return
 */
std::string ResourceManager::getMusicLogs() const
{
    std::string ret = "Printing Music Logs:\n";
    for (auto& itr : mMusic)
        ret += "Music id: " + itr.first + "\n";
    return ret;
}

/**
 * @brief ResourceManager::getChunkLogs
 * @return
 */
std::string ResourceManager::getChunkLogs() const
{
    std::string ret = "Printing Chunk Logs:\n";
    for (auto& itr : mChunks)
        ret += "Chunk id: " + itr.first + "\n";
    return ret;
}

/**
 * @brief ResourceManager::cleanUp
 */
void ResourceManager::cleanUp()
{
    for (auto& mesh : mMeshes)
        mesh.second->cleanUp();

    for (auto& texture : mTextures)
        texture.second->cleanUp();

    for (auto& shader : mShaders)
        shader.second->cleanUp();

    for (auto& music : mMusic)
        music.second->cleanUp();

    for (auto& chunk : mChunks)
        chunk.second->cleanUp();

    mMeshes.clear();
    mTextures.clear();
    mMaterials.clear();
    mShaders.clear();
}

const std::unordered_map<std::string, IMesh::Ptr>& ResourceManager::getMeshes() const
{
    return mMeshes;
}
