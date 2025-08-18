#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes {

struct build_info {
    static inline const std::string CommitSHA = "'86bb240'";
    static inline const std::string Timestamp = "2025-08-18T15:52:41";
    static inline const std::string Version = "7.0.1";
};

}

#endif // buildinfo.h

