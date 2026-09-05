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
        static inline const std::string COMMIT_SHA = "'a4ab2fa'";

        static inline const std::string TIMESTAMP = "2026-09-05T12:44:19";
        
        static inline const std::string VERSION = "8.6.5";
    };

}

#endif // BUILDINFO_H

