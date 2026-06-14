#ifndef CREATE_CONTRACT_H
#define CREATE_CONTRACT_H

#include <MazeBuilder/algos.h>
#include <string_view>

/// @file create_contract.h
/// @namespace mazes
namespace mazes
{
    class randomizer;

    /// @brief Contract for the create function in the runtime
    class create_contract
    {
    public:
        [[nodiscard]] virtual std::string_view create(algo a, unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept = 0;
    };
}

#endif // CREATE_CONTRACT_H
