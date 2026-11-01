#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string_view>
#include <unordered_set>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <ranges>
#include <sstream>
#include <tuple>
#include <vector>

#include <fmt/format.h>

#include <MazeBuilder/barriers.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/output_formats.h>
#include <MazeBuilder/randomizer.h>

using namespace mazes;

std::string string_utils::concat(const std::string &a, const std::string &b) noexcept
{
    return fmt::format("{}{}", a, b);
}

// Construct an acceptable mask source string based on the given pattern.
// The pattern is clamped to a rectangle (rows * cols <= pattern size). BLOCKED cells (case insensitive)
// are extracted as a bit string; its longest repeated sequence seeds the RNG, which samples a bitfield
// that is converted to rows joined by a literal "\\n" (no whitespace, safe for tokenized args).
std::string string_utils::make_mask_source(const std::string &pattern) noexcept
{
    if (pattern.empty())
    {
        return {};
    }

    const auto rows = std::max<std::size_t>(1, static_cast<std::size_t>(std::sqrt(static_cast<double>(pattern.size()))));
    const auto cols = pattern.size() / rows;

    std::string blocked_bits;
    blocked_bits.reserve(rows * cols);
    std::ranges::transform(pattern | std::views::take(rows * cols), std::back_inserter(blocked_bits), [](char c)
                           { return std::toupper(static_cast<unsigned char>(c)) == static_cast<char>(Barrier::BLOCKED) ? '1' : '0'; });

    // Longest repeated (possibly overlapping) sequence, capped for bounded cost
    std::string seed_seq{blocked_bits};
    const auto max_len = std::min<std::size_t>(blocked_bits.size() / 2, 64);
    for (auto len = max_len; len >= 1; --len)
    {
        std::unordered_set<std::string_view> seen;
        const std::string_view bits{blocked_bits};
        bool found = false;
        for (std::size_t i = 0; i + len <= bits.size(); ++i)
        {
            if (!seen.insert(bits.substr(i, len)).second)
            {
                seed_seq = std::string{bits.substr(i, len)};
                found = true;
                break;
            }
        }
        if (found)
        {
            break;
        }
    }

    randomizer rng{};
    rng.seed(std::string_view{seed_seq});

    const auto samples = rng.sample_int_n(0, 1, static_cast<int>(rows * cols));

    std::vector<bool> bitfield(rows * cols, false);
    for (std::size_t i = 0; i < bitfield.size() && i < samples.size(); ++i)
    {
        bitfield[i] = samples[i] != 0;
    }
    // A mask with no open cells cannot be used to build a maze
    if (std::ranges::none_of(bitfield, [](bool b)
                             { return b; }))
    {
        bitfield.front() = true;
    }

    std::string result;
    result.reserve(rows * (cols + 2));
    for (std::size_t r = 0; r < rows; ++r)
    {
        if (r > 0)
        {
            result += "\\n";
        }
        for (std::size_t c = 0; c < cols; ++c)
        {
            result += bitfield[r * cols + c] ? '.' : static_cast<char>(Barrier::BLOCKED);
        }
    }
    return result;
}
