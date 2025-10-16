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
        static inline const std::string CommitSHA = "'6b91db2'";

        static inline const std::string Timestamp = "2025-10-16T06:17:17";
        
        static inline const std::string Version = "7.5.6";
    };

}

#endif // buildinfo.h

