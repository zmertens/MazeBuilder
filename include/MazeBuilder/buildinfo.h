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
        static inline const std::string CommitSHA = "'18f6ee4'";

        static inline const std::string Timestamp = "2025-12-11T09:11:34";
        
        static inline const std::string Version = "7.7.5";
    };

}

#endif // buildinfo.h

