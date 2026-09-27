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
        static inline const std::string COMMIT_SHA = "'a9f7c9d'";

        static inline const std::string TIMESTAMP = "2026-09-26T16:54:10";
        
        static inline const std::string VERSION = "8.6.7";
    };

}

#endif // BUILDINFO_H

