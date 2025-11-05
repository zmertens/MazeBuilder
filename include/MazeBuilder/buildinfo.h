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
        static inline const std::string CommitSHA = "'3be4d97'";

        static inline const std::string Timestamp = "2025-11-03T06:55:46";
        
        static inline const std::string Version = "7.6.6";
    };

}

#endif // buildinfo.h

