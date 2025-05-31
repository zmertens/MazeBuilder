#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes {

struct build_info {
    static inline const std::string CommitSHA = "'dea6126'";
    static inline const std::string Timestamp = "2025-05-31T12:54:28";
    static inline const std::string Version = "6.3.5";
};

}

#endif // buildinfo.h

