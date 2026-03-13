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
        static inline const std::string CommitSHA = "'3b5da0d'";

        static inline const std::string Timestamp = "2026-03-10T15:40:37";
        
        static inline const std::string Version = "7.8.4";
    };

}

#endif // buildinfo.h

