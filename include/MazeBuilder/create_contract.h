#ifndef CREATE_CONTRACT_H
#define CREATE_CONTRACT_H

#include <MazeBuilder/algos.h>
#include <string_view>

/// @file create_contract.h
/// @namespace mazes
namespace mazes
{
    class configurator;
    class randomizer;

    /// @brief Contract for the create function in the runtime
    class create_contract
    {
    public:
        virtual ~create_contract() = default;
        [[nodiscard]] virtual std::string_view create(const configurator& config, randomizer& rng) noexcept = 0;
    };
}

#endif // CREATE_CONTRACT_H
