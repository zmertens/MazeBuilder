#ifndef SINGLETON_BASE_H
#define SINGLETON_BASE_H

#include <memory>

namespace mazes {

template <typename T>
class singleton_base {
public:
    // Deleted copy constructor and assignment operator to prevent copying
    singleton_base(const singleton_base&) = delete;
    singleton_base& operator=(const singleton_base&) = delete;

    // Deleted move constructor and assignment operator to prevent moving
    singleton_base(singleton_base&&) = delete;
    singleton_base& operator=(singleton_base&&) = delete;

    // Static method to access the singleton instance
    template <typename ...Args>
    static std::shared_ptr<T>& instance(Args&&... args) noexcept {
        static std::shared_ptr<T> instance = std::make_shared<T>(std::forward<Args>(args)...);
        return instance;
    }

protected:
    // Private constructor to prevent instantiation from outside the class
    singleton_base() = default;

    // Private destructor to prevent deletion from outside the class
    virtual ~singleton_base() = default;
};

} // namespace mazes

#endif // SINGLETON_BASE_H