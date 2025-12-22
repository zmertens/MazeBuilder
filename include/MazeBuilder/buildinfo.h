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
        static inline const std::string CommitSHA = "'64b6627'";

        static inline const std::string Timestamp = "2025-12-21T08:20:55";
        
        static inline const std::string Version = "7.8.1";
    };

}

#endif // buildinfo.h

