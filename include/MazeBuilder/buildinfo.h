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
        static inline const std::string CommitSHA = "'f4a2fef'";

        static inline const std::string Timestamp = "2026-06-14T04:35:50";
        
        static inline const std::string Version = "8.2.1";
    };

}

#endif // buildinfo.h

