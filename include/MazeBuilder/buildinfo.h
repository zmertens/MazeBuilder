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
        static inline const std::string COMMIT_SHA = "'737daf7'";

        static inline const std::string TIMESTAMP = "2026-08-21T16:34:14";
        
        static inline const std::string VERSION = "8.6.1";
    };

}

#endif // BUILDINFO_H

