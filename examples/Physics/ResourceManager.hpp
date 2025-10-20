#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <memory>
#include <string>
#include <unordered_map>

struct SDL_Texture;
struct SDL_Renderer;

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();
    
    // Configuration management
    bool loadConfiguration(const std::string& configPath);
    std::string getConfigValue(const std::string& key) const;
    const std::unordered_map<std::string, std::string>& getAllConfig() const;
    
    // Texture management
    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& imagePath);
    SDL_Texture* getTexture(const std::string& name) const;
    void clearTextures();
    
    // Audio resource management
    bool loadAudioResources(const std::string& resourcePath);
    
    // Resource path utilities
    std::string getResourcePath(const std::string& filename) const;
    std::string extractJsonValue(const std::string& jsonStr) const;

private:
    struct ResourceManagerImpl;
    std::unique_ptr<ResourceManagerImpl> m_impl;
};

#endif // RESOURCE_MANAGER_HPP

