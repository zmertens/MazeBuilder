#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes {

struct build_info {
    static inline const std::string CommitSHA = "'7e10522'";
    static inline const std::string Timestamp = "2025-02-21T08:01:19";
    static inline const std::string Version = "6.0.1";
};

}

#endif // buildinfo.h

