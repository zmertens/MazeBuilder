#ifndef BATCH_PROCESSOR_H
#define BATCH_PROCESSOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/writer.h>

namespace mazes {

/// @class batch_processor
/// @brief Processes multiple maze configurations in batch
class batch_processor {
public:
    /// @brief Default constructor
    batch_processor() = default;

    /// @brief Process a single maze configuration
    /// @param config The configuration map for a single maze
    /// @param output_file The output file path
    /// @return True if processing was successful
    bool process_single(const std::unordered_map<std::string, std::string>& config, const std::string& output_file);

    /// @brief Process an array of maze configurations
    /// @param configs Vector of configuration maps
    /// @param output_file The output file path
    /// @return True if processing was successful
    bool process_batch(const std::vector<std::unordered_map<std::string, std::string>>& configs, const std::string& output_file);

private:
    /// @brief Extract maze parameters from configuration
    /// @param config The configuration map
    /// @param seed Output seed value
    /// @param algo_str Output algorithm string
    /// @param rows Output rows value
    /// @param columns Output columns value
    /// @param levels Output levels value
    /// @param distances Output distances flag
    /// @param encodes Output encodes flag
    void extract_params(const std::unordered_map<std::string, std::string>& config, 
                       int& seed, std::string& algo_str, 
                       int& rows, int& columns, int& levels, 
                       bool& distances, bool& encodes);

    /// @brief Generate a maze based on configuration
    /// @param seed The random seed
    /// @param algo_str Algorithm name string
    /// @param rows Number of rows
    /// @param columns Number of columns
    /// @param levels Number of levels
    /// @param distances Whether to calculate distances
    /// @param encodes Whether to encode output
    /// @param duration Output parameter for maze generation duration
    /// @return Maze string representation
    std::string generate_maze(int seed, const std::string& algo_str,
                             int rows, int columns, int levels,
                             bool distances, bool encodes,
                             std::chrono::milliseconds& duration);

    /// @brief Handle output for a single maze
    /// @param maze_str The maze string representation
    /// @param output_file The output file path
    /// @param output_type The output format type
    /// @param rows Number of rows
    /// @param columns Number of columns
    /// @param distances Whether distances are calculated
    /// @param duration Maze generation duration
    /// @param append Whether to append to existing file
    /// @return True if output was successful
    bool output_maze(const std::string& maze_str, 
                    const std::string& output_file, 
                    mazes::output output_type,
                    int rows, int columns, bool distances,
                    const std::chrono::milliseconds& duration,
                    bool append = false);

    /// @brief Append results to existing JSON array or create new one
    /// @param results The results to append
    /// @param output_file The output file path
    /// @return True if successful
    bool output_json_batch(const std::vector<std::unordered_map<std::string, std::string>>& results,
                         const std::string& output_file);

    /// @brief Helper to parse a string to int with default value
    /// @param value The string to parse
    /// @param default_value Default value if parsing fails
    /// @return Parsed int value or default
    int parse_int(const std::string& value, int default_value);

    /// @brief Helper to parse a string to bool with default value
    /// @param value The string to parse
    /// @param default_value Default value if parsing fails
    /// @return Parsed bool value or default
    bool parse_bool(const std::string& value, bool default_value);

    /// @brief Clean JSON string value (remove quotes and escapes)
    /// @param value JSON string value
    /// @return Cleaned string
    std::string clean_json_value(const std::string& value);

    /// Storage for batch results
    std::vector<std::unordered_map<std::string, std::string>> batch_results;
};

} // namespace mazes

#endif // BATCH_PROCESSOR_H
