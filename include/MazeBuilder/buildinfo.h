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
        static inline const std::string COMMIT_SHA = "'db468d1'";

        static inline const std::string TIMESTAMP = "2026-11-04T20:25:15";
        
        static inline const std::string VERSION = "8.11.132";
    };

}

#endif // BUILDINFO_H

