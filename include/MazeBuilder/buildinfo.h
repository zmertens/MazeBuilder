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
        static inline const std::string CommitSHA = "'c84ddbc'";

        static inline const std::string Timestamp = "2026-08-10T04:47:46";
        
        static inline const std::string Version = "8.5.5";
    };

}

#endif // BUILDINFO_H

