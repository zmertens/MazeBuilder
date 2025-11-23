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
        static inline const std::string CommitSHA = "'7c34f31'";

        static inline const std::string Timestamp = "2025-11-23T12:35:38";
        
        static inline const std::string Version = "7.6.9";
    };

}

#endif // buildinfo.h

