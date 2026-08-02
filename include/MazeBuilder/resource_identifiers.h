#ifndef RESOURCE_IDENTIFIERS_H
#define RESOURCE_IDENTIFIERS_H

/// @file resource_identifiers.h
/// @namespace mazes
namespace mazes
{
    /// @brief Enumeration of resource identifiers for the maze builder

    enum class args_identifier : unsigned int
    {
        COMMON = 0,
        TOTAL = 1
    };

    enum class processed_text_identifier : unsigned int
    {
        FINISHED = 0,
        GARBAGE = 1,
        PROCESSING = 2,
        TOTAL = 3
    };

    enum class grid_identifier : unsigned int
    {
        BASIC = 0,
        COLORED = 1,
        DISTANCE = 2,
        TOTAL = 3
    };

    class args;
    class grid_interface;
    class processed_text;

    // Forward declaration and a few type definitions
    template <typename Resource, typename Identifier>
    class resource_management;

    typedef resource_management<args, args_identifier> args_manager;
    typedef resource_management<grid_interface, grid_identifier> grid_manager;
    typedef resource_management<processed_text, processed_text_identifier> processed_text_manager;
} // namespace mazes

#endif // RESOURCE_IDENTIFIERS_H
