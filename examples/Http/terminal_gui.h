#ifndef TERMINAL_GUI_H
#define TERMINAL_GUI_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/// @brief Simple terminal GUI for the HTTP maze builder client
class terminal_gui {
public:

    /// @brief Initialize the terminal GUI with the server URL
    /// @param server_url The base URL for Corners server
    void initialize(const std::string& server_url);

    /// @brief Start the main terminal loop
    void run();

    /// @brief Process a single command
    /// @param command The command string to process
    /// @return Command output string
    std::string process_command(const std::string& command);

private:
    /// @brief Display the terminal prompt
    void display_prompt() const;

    /// @brief Register all available commands
    void register_commands();

    /// @brief Parse command line arguments
    /// @param command_line The full command line string
    /// @return Vector of arguments
    std::vector<std::string> parse_arguments(const std::string& command_line) const;

    /// @brief Handle mazebuilderhttp command
    /// @param args Command arguments
    /// @return Command output
    std::string handle_mazebuilderhttp(const std::vector<std::string>& args);

    /// @brief Handle ls command
    /// @param args Command arguments (unused)
    /// @return Command output
    std::string handle_ls(const std::vector<std::string>& args);

    /// @brief Handle find command
    /// @param args Command arguments
    /// @return Command output
    std::string handle_find(const std::vector<std::string>& args);

    /// @brief Handle help command
    /// @param args Command arguments (unused)
    /// @return Command output
    std::string handle_help(const std::vector<std::string>& args);

    /// @brief Handle exit command
    /// @param args Command arguments (unused)
    /// @return Command output
    std::string handle_exit(const std::vector<std::string>& args);

    /// @brief Create a new maze via HTTP request
    /// @param rows Number of rows
    /// @param columns Number of columns
    /// @param seed Random seed
    /// @param algorithm Algorithm to use
    /// @return Response string
    std::string create_maze(int rows, int columns, int seed, const std::string& algorithm);

    /// @brief Display mazebuilderhttp help
    /// @return Help string
    std::string show_mazebuilder_help() const;

    std::string m_server_url;
    std::string m_current_directory;
    bool m_running;

    // Command registry
    std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> m_commands;

    // Available programs in the current directory
    std::vector<std::string> m_available_programs;
};

#endif // TERMINAL_GUI_H
