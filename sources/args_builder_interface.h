#ifndef ARGS_BUILDER_INTERFACE_H
#define ARGS_BUILDER_INTERFACE_H

#include <vector>
#include <string>

namespace mazes {

    class args;

    class args_builder_interface {
    public:
        virtual const args& build() const noexcept = 0;
    private:
        virtual void parse(const std::vector<std::string>& vv) noexcept = 0;
    };

} // namespace mazes

#endif // ARGS_BUILDER_INTERFACE
