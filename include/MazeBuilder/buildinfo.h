#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes {

struct build_info {
    static inline const std::string CommitSHA = "'20ce80e'";
    static inline const std::string Timestamp = "2025-01-11T10:25:44";
    static inline const std::string Version = "6.0.1";
};

}

#endif // buildinfo.h

