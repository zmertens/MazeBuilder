#ifndef JSON_UTILS_HPP
#define JSON_UTILS_HPP

#include <string>
#include <unordered_map>

#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/string_utils.h>

/// @file JsonUtils.hpp
/// @brief Utility functions for JSON handling

class JsonUtils {
public:

    std::string getValue(const std::string& key, const std::unordered_map<std::string, std::string>& resourceMap) const {
        auto it = resourceMap.find(key);
        if (it != resourceMap.end()) {
            return extractJsonValue(it->second);
        }
        return "";
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

    void loadConfiguration(const std::string& configPath, std::unordered_map<std::string, std::string>& resourceMap) {

        using std::runtime_error;
        using std::string;

        mazes::json_helper jh{};

        if (!jh.load(string{configPath}, resourceMap)) {

            throw runtime_error(mazes::string_utils::format("Failed to load JSON configuration from: {}\n", configPath.data()));
        }
    }

};

#endif // JSON_UTILS_HPP

