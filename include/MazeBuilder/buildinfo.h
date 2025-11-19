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
        static inline const std::string CommitSHA = "'601b66b'";

        static inline const std::string Timestamp = "2025-11-19T07:53:03";
        
        static inline const std::string Version = "7.6.6";
    };

}

#endif // buildinfo.h

