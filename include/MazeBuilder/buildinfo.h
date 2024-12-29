#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes {

struct build_info {
    static inline const std::string CommitSHA = "'dfce78a'";
    static inline const std::string Timestamp = "2024-12-28T12:19:06";
    static inline const std::string Version = "5.9.3";
};

}

#endif // buildinfo.h

