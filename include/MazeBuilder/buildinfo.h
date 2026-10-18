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
        static inline const std::string COMMIT_SHA = "'3cd9447'";

        static inline const std::string TIMESTAMP = "2026-10-17T19:24:05";
        
        static inline const std::string VERSION = "8.8.13";
    };

}

#endif // BUILDINFO_H

