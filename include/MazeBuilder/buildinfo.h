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
        static inline const std::string CommitSHA = "'316f58b'";

        static inline const std::string Timestamp = "2025-12-23T07:43:52";
        
        static inline const std::string Version = "7.8.2";
    };

}

#endif // buildinfo.h

