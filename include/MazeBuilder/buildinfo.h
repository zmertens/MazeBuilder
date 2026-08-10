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
        static inline const std::string COMMIT_SHA = "'7b0ad6d'";

        static inline const std::string TIMESTAMP = "2026-08-10T05:25:43";
        
        static inline const std::string VERSION = "8.5.5";
    };

}

#endif // BUILDINFO_H

