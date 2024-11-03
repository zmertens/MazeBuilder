#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes {

struct build_info {
    static inline const std::string CommitSHA = "'232646a'";
    static inline const std::string Timestamp = "2024-11-03T07:56:24";
    static inline const std::string Version = "5.9.3";
};

}

#endif // buildinfo.h

