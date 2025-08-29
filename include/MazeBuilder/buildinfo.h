#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes {

struct build_info {
    static inline const std::string CommitSHA = "'ab38671'";
    static inline const std::string Timestamp = "2025-08-29T14:20:42";
    static inline const std::string Version = "7.1.1";
};

}

#endif // buildinfo.h

