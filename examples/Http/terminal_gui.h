#ifndef TERMINAL_GUI_H
#define TERMINAL_GUI_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
struct IntegerPair;

/// @brief Simple terminal GUI for the HTTP client
class terminal_gui {
public:
    /// @brief Constructor
    terminal_gui();
    
    /// @brief Destructor
    ~terminal_gui();
    
    /// @brief Copy constructor - deleted
    terminal_gui(const terminal_gui& other) = delete;
    
    /// @brief Copy assignment operator - deleted
    terminal_gui& operator=(const terminal_gui& other) = delete;
    
    /// @brief Move constructor
    terminal_gui(terminal_gui&& other) noexcept;
    
    /// @brief Move assignment operator
    terminal_gui& operator=(terminal_gui&& other) noexcept;

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
    std::string handle_maze_client(const std::vector<std::string>& args);

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
    std::string create_maze(int rows, int columns, int seed, const std::string& algorithm, const std::string& distances = {"[0:-1]"});

    /// @brief Display help
    /// @return Help string
    std::string show_help() const;

    std::string m_server_url;
    std::string m_current_directory;
    bool m_running;

    struct terminal_gui_impl;
    std::unique_ptr<terminal_gui_impl> m_terminal_gui_impl;

    // Command registry
    std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> m_commands;

    // Available programs in the current directory
    std::vector<std::string> m_available_programs;
};

#endif // TERMINAL_GUI_H
