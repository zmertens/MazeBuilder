#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

/// @file buildinfo.h
/// @brief Build information for the maze generation library
/// @namespace mazes
namespace mazes
{
    /// @brief Build information for the maze generation library
    struct buildinfo
    {
        static inline const std::string COMMIT_SHA = "'04f1b75'";

        static inline const std::string TIMESTAMP = "2026-10-25T11:58:47";
        
        static inline const std::string VERSION = "8.9.235";
    };

}

#endif // BUILDINFO_H

