#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <ostream>

namespace mazes {
    namespace tools {


        template <typename Ptr>
        static std::string stringify(Ptr const& p) noexcept {
            std::ostringstream oss;
            oss << *p.get();
            return oss.str();
        }
    }
}


#endif // TOOLS_H
