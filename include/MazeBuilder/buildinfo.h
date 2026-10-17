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
        static inline const std::string COMMIT_SHA = "'00b8089'";

        static inline const std::string TIMESTAMP = "2026-10-17T16:27:38";
        
        static inline const std::string VERSION = "8.8.12";
    };

}

#endif // BUILDINFO_H

