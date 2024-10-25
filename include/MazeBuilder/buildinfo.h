#ifndef BUILDINFO_H
#define BUILDINFO_H

#include <string>

namespace mazes {

struct build_info {
    static inline const std::string CommitSHA = "'f4046fe'";
    static inline const std::string Timestamp = "2024-10-25T11:03:59";
    static inline const std::string Version = "5.2.8";
};

}

#endif // buildinfo.h

