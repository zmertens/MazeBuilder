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
        static inline const std::string COMMIT_SHA = "'671fe2d'";

        static inline const std::string TIMESTAMP = "2026-11-01T08:55:29";
        
        static inline const std::string VERSION = "8.10.23";
    };

}

#endif // BUILDINFO_H

