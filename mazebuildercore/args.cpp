#include <MazeBuilder/args.h>

#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/parsing_state.h>

#include <fmt/format.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace mazes;

using arg_map = args::arg_map;

class args::impl
{
public:
    impl() = default;

    std::vector<arg_map> arguments;

    // Stores the joined tokens as a single raw map; parsing_state turns it into real arguments
    bool parse(const std::vector<std::string> &args_vec, const bool has_program_name)
    {
        std::ostringstream joined;
        for (std::size_t i = has_program_name ? 1 : 0; i < args_vec.size(); ++i)
        {
            joined << (joined.tellp() > 0 ? " " : "") << args_vec[i];
        }

        if (joined.str().empty())
        {
            arguments.clear();
            return false;
        }

        arguments = {arg_map{{args::RAW_WORD_STR, joined.str()}}};
        return true;
    }

    void clear() noexcept
    {
        arguments.clear();
    }
};

args::args() noexcept : pimpl{std::make_unique<impl>()}
{
}

args::~args() = default;

args::args(args &&other) noexcept = default;

args &args::operator=(args &&other) noexcept = default;

args::args(const args &other) : pimpl{std::make_unique<impl>()}
{
    if (other.pimpl)
    {
        pimpl->arguments = other.pimpl->arguments;
    }
}

args &args::operator=(const args &other)
{
    if (this != &other && other.pimpl)
    {
        pimpl = std::make_unique<impl>();
        pimpl->arguments = other.pimpl->arguments;
    }
    return *this;
}

bool args::parse(const std::vector<std::string> &arguments, const bool has_program_name_as_first_arg) const noexcept
{
    if (arguments.empty())
    {
        return false;
    }

    try
    {
        return pimpl->parse(arguments, has_program_name_as_first_arg);
    }
    catch (...)
    {
        return false;
    }
}

bool args::parse(const std::string &arguments, const bool has_program_name_as_first_arg) const noexcept
{
    if (arguments.empty())
    {
        return false;
    }

    try
    {
        std::vector<std::string> tokens;
        std::istringstream iss(arguments);
        std::string token;

        while (iss >> std::quoted(token))
        {
            tokens.push_back(token);
        }

        return parse(tokens, has_program_name_as_first_arg);
    }
    catch (...)
    {
        return false;
    }
}

bool args::parse(int argc, char **argv, const bool has_program_name_as_first_arg) const noexcept
{
    if (argc <= 0 || !argv)
    {
        return false;
    }

    try
    {
        std::vector<std::string> args_vec;
        args_vec.reserve(argc);

        for (int i = 0; i < argc; ++i)
        {
            if (argv[i])
            {
                args_vec.emplace_back(argv[i]);
            }
        }

        return parse(args_vec, has_program_name_as_first_arg);
    }
    catch (...)
    {
        return false;
    }
}

void args::set(std::vector<arg_map> maps) const noexcept
{
    if (pimpl)
    {
        pimpl->arguments = std::move(maps);
    }
}

void args::clear() const noexcept
{
    if (pimpl)
    {
        pimpl->clear();
    }
}

std::optional<std::string> args::get(const std::string &key) const noexcept
{
    if (!pimpl || pimpl->arguments.empty())
    {
        return std::nullopt;
    }

    // Search in first map (command-line args or first JSON object)
    const auto &map = pimpl->arguments.front();
    if (const auto it = map.find(key); it != map.cend())
    {
        return it->second;
    }

    return std::nullopt;
}

arg_map args::front() const noexcept
{
    if (!pimpl || pimpl->arguments.empty())
    {
        return {};
    }

    return pimpl->arguments.front();
}

bool args::pop_front() const noexcept
{
    if (!pimpl || pimpl->arguments.empty())
    {
        return false;
    }

    pimpl->arguments.erase(pimpl->arguments.begin());
    return true;
}

std::vector<arg_map> args::get() const noexcept
{
    if (!pimpl || pimpl->arguments.empty())
    {
        return {};
    }

    return pimpl->arguments;
}

[[nodiscard]] std::size_t args::count() const noexcept
{
    if (!pimpl)
    {
        return 0;
    }

    return pimpl->arguments.size();
}
