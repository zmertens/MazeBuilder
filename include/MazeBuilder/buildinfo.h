#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes {

struct build_info {
    static inline const std::string CommitSHA = "'4ec880d'";
    static inline const std::string Timestamp = "2025-04-05T13:50:18";
    static inline const std::string Version = "6.0.1";
};

}

#endif // buildinfo.h

