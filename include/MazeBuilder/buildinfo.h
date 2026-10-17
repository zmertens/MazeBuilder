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
        static inline const std::string COMMIT_SHA = "'fe8f368'";

        static inline const std::string TIMESTAMP = "2026-10-17T15:01:44";
        
        static inline const std::string VERSION = "8.7.4";
    };

}

#endif // BUILDINFO_H

