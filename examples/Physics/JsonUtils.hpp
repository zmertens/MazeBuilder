#ifndef JSON_UTILS_HPP
#define JSON_UTILS_HPP

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>

#include <SDL3/SDL.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/string_utils.h>

/// @file JsonUtils.hpp
/// @brief Utility functions for JSON handling

class JSONUtils
{
public:
    [[nodiscard]] std::string getValue(const std::string& key, const std::unordered_map<std::string, std::string>& resourceMap) const
    {
        auto it = resourceMap.find(key);
        if (it != resourceMap.end())
        {
            return extractJsonValue(it->second);
        }
        return "";
    }

    // Extract the actual filename from JSON string format (remove quotes and array brackets)
    [[nodiscard]] static std::string extractJsonValue(const std::string& jsonStr)
    {
        if (jsonStr.empty())
        {
            return "";
        }

        std::string result = jsonStr;

        // Remove array brackets if present
        if (result.length() >= 2 && result.front() == '[' && result.back() == ']')
        {
            result = result.substr(1, result.length() - 2);
        }

        // Remove quotes if present
        if (result.length() >= 2 && result.front() == '"' && result.back() == '"')
        {
            result = result.substr(1, result.length() - 2);
        }

        // If it's still an array, just take the first element
        size_t commaPos = result.find(',');
        if (commaPos != std::string::npos)
        {
            result = result.substr(0, commaPos);
            // Remove quotes from the first element
            if (result.length() >= 2 && result.front() == '"' && result.back() == '"')
            {
                result = result.substr(1, result.length() - 2);
            }
        }

        return result;
    }

    static void loadConfiguration(const std::string& configPath, std::unordered_map<std::string, std::string>& resourceMap)
    {
        using std::runtime_error;
        using std::string;

        mazes::json_helper jh{};

        if (!jh.load(string{configPath}, resourceMap))
        {
            throw runtime_error(
                mazes::string_utils::format("Failed to load JSON configuration from: {}\n", configPath.data()));
        }
    }

    /// @brief Convert JSON object string to mazes::configurator
    /// @param jsonValue JSON string in format: {"rows": 100, "columns": 99, "seed": 50, "algo": "dfs"}
    /// @return Configured mazes::configurator object
    static mazes::configurator jsonToConfigurator(const std::string& jsonValue)
    {
        using mazes::args;
        using mazes::configurator;
        using mazes::to_algo_from_sv;

        configurator config;

        // Extract rows
        if (auto rowsValue = extractJsonIntField(jsonValue, args::ROW_WORD_STR))
        {
            config.rows(static_cast<unsigned int>(rowsValue.value()));
        }

        // Extract columns
        if (auto columnsValue = extractJsonIntField(jsonValue, args::COLUMN_WORD_STR))
        {
            config.columns(static_cast<unsigned int>(columnsValue.value()));
        }

        // Extract seed
        if (auto seedValue = extractJsonIntField(jsonValue, args::SEED_WORD_STR))
        {
            config.seed(static_cast<unsigned int>(seedValue.value()));
        }

        // Extract algo
        if (auto algoValue = extractJsonStringField(jsonValue, args::ALGO_ID_WORD_STR))
        {
            config.algo_id(to_algo_from_sv(algoValue.value()));
        }

        return config;
    }

private:
    /// @brief Extract an integer field from JSON string
    /// @param jsonValue The JSON string to parse
    /// @param fieldName The field name to extract
    /// @return Optional containing the integer value if found and valid
    [[nodiscard]] static std::optional<int> extractJsonIntField(const std::string& jsonValue, const std::string& fieldName)
    {
        std::string searchKey = "\"" + fieldName + "\"";
        auto fieldPos = jsonValue.find(searchKey);

        if (fieldPos == std::string::npos)
        {
            return std::nullopt;
        }

        auto colonPos = jsonValue.find(':', fieldPos);
        if (colonPos == std::string::npos)
        {
            return std::nullopt;
        }

        auto commaPos = jsonValue.find(',', colonPos);
        if (commaPos == std::string::npos)
        {
            commaPos = jsonValue.find('}', colonPos);
        }

        if (commaPos == std::string::npos)
        {
            return std::nullopt;
        }

        auto value = jsonValue.substr(colonPos + 1, commaPos - colonPos - 1);

        // Remove whitespace
        value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

        try
        {
            return std::stoi(value);
        }
        catch (...)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to parse %s from: %s\n",
                         fieldName.c_str(), value.c_str());
            return std::nullopt;
        }
    }

    /// @brief Extract a string field from JSON string
    /// @param jsonValue The JSON string to parse
    /// @param fieldName The field name to extract
    /// @return Optional containing the string value if found
    [[nodiscard]] static std::optional<std::string> extractJsonStringField(const std::string& jsonValue, const std::string& fieldName)
    {
        std::string searchKey = "\"" + fieldName + "\"";
        auto fieldPos = jsonValue.find(searchKey);

        if (fieldPos == std::string::npos)
        {
            return std::nullopt;
        }

        auto colonPos = jsonValue.find(':', fieldPos);
        if (colonPos == std::string::npos)
        {
            return std::nullopt;
        }

        auto quoteStart = jsonValue.find('"', colonPos);
        if (quoteStart == std::string::npos)
        {
            return std::nullopt;
        }

        auto quoteEnd = jsonValue.find('"', quoteStart + 1);
        if (quoteEnd == std::string::npos)
        {
            return std::nullopt;
        }

        return jsonValue.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
    }
};

#endif // JSON_UTILS_HPP
