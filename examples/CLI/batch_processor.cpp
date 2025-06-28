#include "batch_processor.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <optional>
#include <fstream>
#include <sstream>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;

bool batch_processor::process_single(const std::unordered_map<std::string, std::string>& config, const std::string& output_file) {
    int seed = 0;
    std::string algo_str = "dfs";
    int rows = 3, columns = 2, levels = 1;
    bool distances = false;
    bool encodes = false;

    extract_params(config, seed, algo_str, rows, columns, levels, distances, encodes);

    std::chrono::milliseconds duration;
    std::string maze_str = generate_maze(seed, algo_str, rows, columns, levels, distances, encodes, duration);
    
    if (maze_str.empty()) {
        return false;
    }

    mazes::output output_type;
    try {
        output_type = mazes::to_output_from_string(output_file.substr(output_file.find_last_of(".") + 1));
    } catch (const std::invalid_argument&) {
        output_type = mazes::output::STDOUT;
    }

    return output_maze(maze_str, output_file, output_type, rows, columns, distances, duration, false);
}

bool batch_processor::process_batch(const std::vector<std::unordered_map<std::string, std::string>>& configs, const std::string& output_file) {
    if (configs.empty()) {
        return false;
    }

    // Clear previous batch results
    batch_results.clear();
    
    // Determine output type
    mazes::output output_type;
    try {
        output_type = mazes::to_output_from_string(output_file.substr(output_file.find_last_of(".") + 1));
    } catch (const std::invalid_argument&) {
        output_type = mazes::output::STDOUT;
    }
    
    // If JSON output, collect all results and output at once
    bool json_output = (output_type == mazes::output::JSON);
    
    // Process each configuration
    bool first_item = true;
    bool success = true;
    
    for (const auto& config : configs) {
        int seed = 0;
        std::string algo_str = "dfs";
        int rows = 3, columns = 2, levels = 1;
        bool distances = false;
        bool encodes = false;

        extract_params(config, seed, algo_str, rows, columns, levels, distances, encodes);

        std::chrono::milliseconds duration;
        std::string maze_str = generate_maze(seed, algo_str, rows, columns, levels, distances, encodes, duration);
        
        if (maze_str.empty()) {
            success = false;
            continue;
        }

        if (json_output) {
            // For JSON output, collect results to output all at once
            std::unordered_map<std::string, std::string> result = config;
            result["duration"] = std::to_string(duration.count());
            result["str"] = maze_str;
            batch_results.push_back(result);
        } else {
            // For other output types, output each maze as we go
            bool append = !first_item; // Append for all except first
            if (!output_maze(maze_str, output_file, output_type, rows, columns, distances, duration, append)) {
                success = false;
            }
        }
        
        first_item = false;
    }
    
    // If JSON output, process all collected results at once
    if (json_output && !batch_results.empty()) {
        return output_json_batch(batch_results, output_file);
    }
    
    return success;
}

void batch_processor::extract_params(const std::unordered_map<std::string, std::string>& config, 
                                    int& seed, std::string& algo_str, 
                                    int& rows, int& columns, int& levels, 
                                    bool& distances, bool& encodes) {
    // Extract parameters with proper parsing
    auto it = config.find("seed");
    if (it != config.end()) {
        seed = parse_int(it->second, 0);
    }
    
    it = config.find("algo");
    if (it != config.end()) {
        algo_str = clean_json_value(it->second);
    }
    
    it = config.find("rows");
    if (it != config.end()) {
        rows = parse_int(it->second, 3);
    }
    
    it = config.find("columns");
    if (it != config.end()) {
        columns = parse_int(it->second, 2);
    }
    
    it = config.find("levels");
    if (it != config.end()) {
        levels = parse_int(it->second, 1);
    }
    
    it = config.find("distances");
    if (it != config.end()) {
        distances = parse_bool(it->second, false);
    }
    
    it = config.find("encode");
    if (it != config.end()) {
        encodes = parse_bool(it->second, false);
    }
}

std::string batch_processor::generate_maze(int seed, const std::string& algo_str,
                                         int rows, int columns, int levels,
                                         bool distances, bool encodes,
                                         std::chrono::milliseconds& duration) {
    using namespace std;
    
    try {
        mazes::algo maze_type = mazes::to_algo_from_string(algo_str);
        static constexpr auto BLOCK_ID = -1;
        
        using maze_ptr = optional<unique_ptr<mazes::maze>>;
        
        mazes::progress<chrono::milliseconds, chrono::high_resolution_clock> clock;
        clock.start();
        
        maze_ptr next_maze_ptr = mazes::factory::create(
            mazes::configurator().columns(columns).rows(rows).levels(levels)
            .distances(distances).seed(seed).algo_id(maze_type)
            .block_id(BLOCK_ID));
            
        duration = clock.elapsed<chrono::milliseconds>();
        
        if (!next_maze_ptr.has_value()) {
            throw runtime_error("Failed to create maze");
        }
        
        // Get the string representation from the maze
        std::string temp_s{ next_maze_ptr.value()->str() };
        
        if (encodes) {
            mazes::base64_helper my_base64;
            return my_base64.encode(temp_s);
        }
        
        return temp_s;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error generating maze: " << ex.what() << std::endl;
        return "";
    }
}

bool batch_processor::output_maze(const std::string& maze_str, 
                                 const std::string& output_file, 
                                 mazes::output output_type,
                                 int rows, int columns, bool distances,
                                 const std::chrono::milliseconds& duration,
                                 bool append) {
    using namespace std;
    
    bool success = false;
    mazes::writer my_writer;
    
    try {
        switch (output_type) {
        case mazes::output::WAVEFRONT_OBJECT_FILE: {
            vector<tuple<int, int, int, int>> vertices;
            vector<vector<uint32_t>> faces;
            mazes::wavefront_object_helper woh{};
            auto obj_str = woh.to_wavefront_object_str(vertices, faces);
            success = my_writer.write_file(output_file, obj_str);
            break;
        }
        case mazes::output::PNG: {
            static constexpr auto STRIDE = 4u;
            vector<uint8_t> pixels;
            auto pixels_w = 0, pixels_h = 0;
            pixels.reserve(rows * columns * STRIDE);
            if (distances) {
                // mazes::stringz::to_pixels_colored(maze_str, pixels, pixels_w, pixels_h, STRIDE);
            } else {
                // mazes::stringz::to_pixels(maze_str, pixels, pixels_w, pixels_h, STRIDE);
            }
            success = my_writer.write_png(output_file, pixels, pixels_w, pixels_h, STRIDE);
            break;
        }
        case mazes::output::JPEG: {
            static constexpr auto STRIDE = 4u;
            vector<uint8_t> pixels;
            auto pixels_w = 0, pixels_h = 0;
            pixels.reserve(rows * columns * STRIDE);
            if (distances) {
                // mazes::stringz::to_pixels_colored(maze_str, pixels, pixels_w, pixels_h, STRIDE);
            } else {
                // mazes::stringz::to_pixels(maze_str, pixels, pixels_w, pixels_h, STRIDE);
            }
            success = my_writer.write_jpeg(output_file, pixels, pixels_w, pixels_h, STRIDE);
            break;
        }
        case mazes::output::JSON: {
            // For JSON single output, not batch - handled separately
            mazes::json_helper jh{};
            std::unordered_map<std::string, std::string> result;
            result["rows"] = std::to_string(rows);
            result["columns"] = std::to_string(columns);
            result["duration"] = std::to_string(duration.count());
            result["str"] = maze_str;
            const auto& args_to_json_str = jh.from(result);
            success = my_writer.write_file(output_file, args_to_json_str);
            break;
        }
        case mazes::output::PLAIN_TEXT: {
            std::string content = maze_str;
            if (append) {
                content = "\n\n" + content; // Add separator for multiple mazes
            }
            if (append) {
                std::ofstream ofs(output_file, std::ios_base::app);
                if (ofs) {
                    ofs << content;
                    success = true;
                }
            } else {
                success = my_writer.write_file(output_file, content);
            }
            break;
        }
        case mazes::output::STDOUT: {
            std::cout << maze_str << std::endl;
            if (!append) {
                std::cout << std::endl; // Add separator between mazes
            }
            success = true;
            break;
        }
        default:
            success = false;
        }
        
#if defined(MAZE_DEBUG)
        if (success) {
            std::cout << "Writing to file: " << output_file << std::endl;
            std::cout << "Duration: " << fixed << setprecision(3) << duration.count() << " milliseconds" << std::endl;
        } else {
            std::cerr << "Writing to: " << output_file << " failed!" << std::endl;
        }
#endif
        
        return success;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error in output_maze: " << ex.what() << std::endl;
        return false;
    }
}

bool batch_processor::output_json_batch(const std::vector<std::unordered_map<std::string, std::string>>& results,
                                      const std::string& output_file) {
    try {
        // Use the enhanced json_helper class to convert vector of maps to JSON array
        mazes::json_helper jh{};
        std::string json_str = jh.from(results, 4); // Using 4 spaces for indentation
        
        // Write the JSON string to file
        mazes::writer my_writer;
        return my_writer.write_file(output_file, json_str);
    }
    catch (const std::exception& ex) {
        std::cerr << "Error in output_json_batch: " << ex.what() << std::endl;
        return false;
    }
}

int batch_processor::parse_int(const std::string& value, int default_value) {
    try {
        // Strip quotes from JSON numeric values
        std::string clean_val = clean_json_value(value);
        return std::stoi(clean_val);
    } catch (...) {
        return default_value;
    }
}

bool batch_processor::parse_bool(const std::string& value, bool default_value) {
    try {
        std::string clean_val = clean_json_value(value);
        std::transform(clean_val.begin(), clean_val.end(), clean_val.begin(), 
                      [](unsigned char c) { return std::tolower(c); });
                      
        if (clean_val == "true" || clean_val == "1") {
            return true;
        } else if (clean_val == "false" || clean_val == "0") {
            return false;
        }
        return default_value;
    } catch (...) {
        return default_value;
    }
}

std::string batch_processor::clean_json_value(const std::string& value) {
    if (value.empty()) {
        return value;
    }
    
    // Remove leading and trailing whitespace
    size_t start = value.find_first_not_of(" \t\r\n");
    size_t end = value.find_last_not_of(" \t\r\n");
    
    if (start == std::string::npos || end == std::string::npos) {
        return value;
    }
    
    std::string result = value.substr(start, end - start + 1);
    
    // Remove quotes if present
    if (result.size() >= 2 && result.front() == '\"' && result.back() == '\"') {
        result = result.substr(1, result.size() - 2);
    }
    
    return result;
}
