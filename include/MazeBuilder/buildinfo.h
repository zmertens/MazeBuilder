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
        static inline const std::string CommitSHA = "'bab9826'";

        static inline const std::string Timestamp = "2025-10-03T07:13:24";
        
        static inline const std::string Version = "7.2.6";
    };

}

#endif // buildinfo.h

