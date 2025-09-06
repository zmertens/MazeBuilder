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
        static inline const std::string CommitSHA = "'bfb23b1'";

        static inline const std::string Timestamp = "2025-09-06T14:39:39";
        
        static inline const std::string Version = "7.2.5";
    };

}

#endif // buildinfo.h

