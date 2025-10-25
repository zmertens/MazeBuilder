#include "PhysicsResourceManager.hpp"

#include <iostream>
#include <unordered_map>
#include <string>

#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/string_utils.h>

#include "Texture.hpp"

class PhysicsResourceManager::PhysicsResourceManagerImpl {
public:
    // Configuration data loaded
    std::unordered_map<std::string, std::string> resourceMap;
    
    // Resource path prefix
    static const std::string COMMON_RESOURCE_PATH_PREFIX;

    std::optional<PhysicsResources> initializeAllResources(std::string_view configPath) {

        using std::cerr;

        PhysicsResources resources;
        
        if (!loadConfiguration(configPath)) {

#if defined(MAZE_DEBUG)

            cerr << mazes::string_utils::format("Failed to load configuration from: {}\n", configPath.data());
#endif

            return std::nullopt;
        }
        
        // Load splash texture path only (actual texture loading will be done by game)
        std::string splashPath = getConfigValue("splash_image");
        resources.splashPath = getResourcePath(splashPath);
        
        // Load music and sound paths
        resources.musicPath = getConfigValue("music_wav");
        resources.soundPath = getConfigValue("music_ogg");
        
        resources.windowIconPath = getResourcePath(getConfigValue("icon_image"));

        // Success if we have basic paths loaded
        resources.success = (!resources.windowIconPath.empty() && !resources.splashPath.empty());

        return resources;
    }

private:

    std::string getConfigValue(const std::string& key) const {
        auto it = resourceMap.find(key);
        if (it != resourceMap.end()) {
            return extractJsonValue(it->second);
        }
        return "";
    }

    std::string getResourcePath(const std::string& filename) const {
        return COMMON_RESOURCE_PATH_PREFIX + "/" + filename;
    }
    
    // Extract the actual filename from JSON string format (remove quotes and array brackets)
    std::string extractJsonValue(const std::string& jsonStr) const {
        if (jsonStr.empty()) {
            return "";
        }
        
        std::string result = jsonStr;
        
        // Remove array brackets if present
        if (result.length() >= 2 && result.front() == '[' && result.back() == ']') {
            result = result.substr(1, result.length() - 2);
        }
        
        // Remove quotes if present
        if (result.length() >= 2 && result.front() == '"' && result.back() == '"') {
            result = result.substr(1, result.length() - 2);
        }
        
        // If it's still an array, just take the first element
        size_t commaPos = result.find(',');
        if (commaPos != std::string::npos) {
            result = result.substr(0, commaPos);
            // Remove quotes from the first element
            if (result.length() >= 2 && result.front() == '"' && result.back() == '"') {
                result = result.substr(1, result.length() - 2);
            }
        }
        
        return result;
    }

    bool loadConfiguration(std::string_view configPath) {

        using std::cerr;
        using std::cout;
        using std::string;

        mazes::json_helper jh{};

        if (!jh.load(string{configPath}, resourceMap)) {

#if defined(MAZE_DEBUG)
      
            cerr << mazes::string_utils::format("Failed to load JSON configuration from: {}\n", configPath.data());
#endif
            return false;
        }
        
#if defined(MAZE_DEBUG)

        cout << mazes::string_utils::format("Successfully loaded configuration from: {}\n", configPath.data());

        // Log loaded configuration for debugging
        for (const auto& [key, value] : resourceMap) {

            cout << mazes::string_utils::format("Config Key: {} | Value: {}\n", key.c_str(), value.c_str());
        }
#endif
        
        return true;
    }
};

const std::string PhysicsResourceManager::PhysicsResourceManagerImpl::COMMON_RESOURCE_PATH_PREFIX = "resources";

// PhysicsResourceManager implementation
PhysicsResourceManager::PhysicsResourceManager() : m_impl(std::make_unique<PhysicsResourceManagerImpl>()) {

}

PhysicsResourceManager::~PhysicsResourceManager() = default;

std::optional<PhysicsResourceManager::PhysicsResources> PhysicsResourceManager::initializeAllResources(std::string_view configPath) {

    return this->m_impl->initializeAllResources(configPath);
}
