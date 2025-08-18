#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes {

struct build_info {
    static inline const std::string CommitSHA = "'1061001'";
    static inline const std::string Timestamp = "2025-06-28T14:08:06";
    static inline const std::string Version = "6.7.5";
};

}

#endif // buildinfo.h

