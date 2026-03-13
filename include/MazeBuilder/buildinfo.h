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
        static inline const std::string CommitSHA = "'5d4137c'";

        static inline const std::string Timestamp = "2026-03-12T18:08:44";
        
        static inline const std::string Version = "7.9.0";
    };

}

#endif // buildinfo.h

