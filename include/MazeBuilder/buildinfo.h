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
        static inline const std::string CommitSHA = "'f1b1ef5'";

        static inline const std::string Timestamp = "2026-07-25T15:58:43";
        
        static inline const std::string Version = "8.5.4";
    };

}

#endif // BUILDINFO_H

