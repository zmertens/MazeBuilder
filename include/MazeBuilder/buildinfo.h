#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes 
{

    struct build_info
    {
        static inline const std::string CommitSHA = "'6b77f2a'";

        static inline const std::string Timestamp = "2025-09-03T00:49:25";
        
        static inline const std::string Version = "7.2.1";
    };

}

#endif // buildinfo.h

