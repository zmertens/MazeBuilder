#include "ResourceManager.hpp"

#include <SDL3/SDL.h>
#include <unordered_map>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <MazeBuilder/json_helper.h>

class ResourceManager::ResourceManagerImpl {
public:
    // Configuration loaded from physics.json
    std::unordered_map<std::string, std::string> resourceMap;
    
    // Texture cache
    std::unordered_map<std::string, SDL_Texture*> textureCache;
    
    // Resource path prefix
    static const std::string RESOURCE_PATH_PREFIX;
    
    ResourceManagerImpl() = default;
    
    ~ResourceManagerImpl() {
        clearTextures();
    }
    
    void clearTextures() {
        for (auto& [name, texture] : textureCache) {
            if (texture) {
                SDL_DestroyTexture(texture);
            }
        }
        textureCache.clear();
    }
    
    // Load an image file using stb_image and create an SDL texture
    SDL_Texture* loadImageTexture(SDL_Renderer* renderer, const std::string& imagePath) {
        int width, height, channels;
        unsigned char* imageData = stbi_load(imagePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        
        if (!imageData) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load image: %s - %s\n", imagePath.c_str(), stbi_failure_reason());
            return nullptr;
        }
        
        // Create surface from image data (force RGBA format)
        SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA8888, imageData, width * 4);
        if (!surface) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create surface: %s\n", SDL_GetError());
            stbi_image_free(imageData);
            return nullptr;
        }
        
        // Create texture from surface
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture: %s\n", SDL_GetError());
        }
        
        // Clean up
        SDL_DestroySurface(surface);
        stbi_image_free(imageData);
        
        return texture;
    }
    
    // Extract the actual filename from JSON string format (remove quotes and array brackets)
    std::string extractJsonValue(const std::string& jsonStr) const {
        std::string result = jsonStr;
        
        // Remove array brackets if present
        if (result.front() == '[' && result.back() == ']') {
            result = result.substr(1, result.length() - 2);
        }
        
        // Remove quotes if present
        if (result.front() == '"' && result.back() == '"') {
            result = result.substr(1, result.length() - 2);
        }
        
        // If it's still an array, just take the first element
        size_t commaPos = result.find(',');
        if (commaPos != std::string::npos) {
            result = result.substr(0, commaPos);
            // Remove quotes from the first element
            if (result.front() == '"' && result.back() == '"') {
                result = result.substr(1, result.length() - 2);
            }
        }
        
        return result;
    }
};

const std::string ResourceManager::ResourceManagerImpl::RESOURCE_PATH_PREFIX = "resources";

// ResourceManager implementation
ResourceManager::ResourceManager() : m_impl(std::make_unique<ResourceManagerImpl>()) {
}

ResourceManager::~ResourceManager() = default;

bool ResourceManager::loadConfiguration(const std::string& configPath) {
    mazes::json_helper jh{};
    if (!jh.load(configPath, m_impl->resourceMap)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load configuration file: %s", configPath.c_str());
        return false;
    }
    
    SDL_Log("Successfully loaded configuration with %zu entries\n", m_impl->resourceMap.size());
    
    // Log loaded configuration for debugging
    for (const auto& [key, value] : m_impl->resourceMap) {
        SDL_Log("Config: %s = %s", key.c_str(), value.c_str());
    }
    
    return true;
}

std::string ResourceManager::getConfigValue(const std::string& key) const {
    auto it = m_impl->resourceMap.find(key);
    return (it != m_impl->resourceMap.end()) ? it->second : "";
}

const std::unordered_map<std::string, std::string>& ResourceManager::getAllConfig() const {
    return m_impl->resourceMap;
}

SDL_Texture* ResourceManager::loadTexture(SDL_Renderer* renderer, const std::string& imagePath) {
    // Check cache first
    auto it = m_impl->textureCache.find(imagePath);
    if (it != m_impl->textureCache.end()) {
        return it->second;
    }
    
    // Load new texture
    SDL_Texture* texture = m_impl->loadImageTexture(renderer, imagePath);
    if (texture) {
        m_impl->textureCache[imagePath] = texture;
    }
    
    return texture;
}

SDL_Texture* ResourceManager::getTexture(const std::string& name) const {
    auto it = m_impl->textureCache.find(name);
    return (it != m_impl->textureCache.end()) ? it->second : nullptr;
}

void ResourceManager::clearTextures() {
    m_impl->clearTextures();
}

bool ResourceManager::loadAudioResources(const std::string& resourcePath) {
    // Audio resource loading logic can be implemented here
    // For now, just return true as audio loading is handled by SDLHelper
    return true;
}

std::string ResourceManager::getResourcePath(const std::string& filename) const {
    return ResourceManager::ResourceManagerImpl::RESOURCE_PATH_PREFIX + "/" + filename;
}

std::string ResourceManager::extractJsonValue(const std::string& jsonStr) const {
    return m_impl->extractJsonValue(jsonStr);
}