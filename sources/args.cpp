#include <MazeBuilder/args.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/writer.h>

#include <string>
#include <functional>
#include <algorithm>
#include <unordered_map>

#include <CLI11/CLI11.hpp>

using namespace mazes;

struct args::args_impl {

private:
    CLI::App cli_app;

public:
    explicit args_impl() : cli_app{ "maze_builder" } {}

    void add_flag(const std::string& key, bool& r, const std::string& desc) noexcept {
        cli_app.add_flag(key, r, desc);
    }

    void add_option(const std::string& key, const std::string& desc) noexcept {
        cli_app.add_option(key, desc);
    }

    std::vector<const CLI::Option*> get_options() const noexcept {
        return cli_app.get_options();
    }

    bool parse(int argc, char* argv[], const std::vector<std::string>& flags = {"-h", "-v"}) noexcept {
        // Copy the flags and options to the CLI program
        for (const auto& flag : flags) {
            cli_app.add_flag(flag);
        }

        try {
            cli_app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            return false;
        }
        return true;
    }

    bool parse(const std::string& arguments) noexcept {
        std::vector<std::string> args = CLI::detail::split_up(arguments);
        std::vector<char*> argv;
        for (auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        return parse(static_cast<int>(argv.size()), argv.data());
    }

    void clear() noexcept {
        cli_app.clear();
    }
};

args::args() : pimpl{ std::make_unique<args_impl>() } {}

args::~args() = default;

/// @brief Add a flag to the args map
/// @param key 
/// @param desc 
void args::add_flag(const std::string& key, bool& r, const std::string& desc) noexcept {
    this->pimpl->add_flag(key, std::ref(r), desc);
}

/// @brief Add an option to the args map
/// @param key 
/// @param desc 
void args::add_option(const std::string& key, const std::string& desc) noexcept {
    this->pimpl->add_option(key, desc);
}

bool args::parse(int argc, char* argv[]) noexcept {
    return this->pimpl->parse(argc, argv);
}

bool args::parse(const std::vector<std::string>& arguments) noexcept {
    using namespace std;

    if (arguments.empty()) {
        return false;
    }

    vector<char*> argv;
    transform(arguments.cbegin(), arguments.cend(), back_inserter(argv),
        [](const string& arg) {
            return const_cast<char*>(arg.c_str());
        });

    return this->pimpl->parse(static_cast<int>(argv.size()), argv.data());
}

bool args::parse(const std::string& arguments) noexcept {
    using namespace std;

    if (arguments.empty()) {
        return false;
    }

    return this->pimpl->parse(cref(arguments));
}

void args::clear() noexcept {
    this->pimpl->clear();
}

/// @brief Dump the hash table to a string output
/// @return 
std::ostream& mazes::operator<<(std::ostream& os, const args& a) noexcept {
    if (!a.pimpl || !os.good()) {
        return os;
    }

    using namespace std;
    // Copy the CLI program to a map
    unordered_map<string, string> args_map;

    const auto& options = a.pimpl->get_options();
    transform(options.cbegin(), options.cend(),
        inserter(args_map, args_map.end()),
        [](const CLI::Option* cli_opt) {
            return make_pair(cli_opt->get_single_name(), cli_opt->get_description());
        });

    json_helper jh{};
    os << jh.from(args_map);
    return os;
}
