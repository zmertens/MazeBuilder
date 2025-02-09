#ifndef BASE64_HELPER_H
#define BASE64_HELPER_H

#include <memory>
#include <string>

namespace mazes {

    class maze;

class base_64_helper {
public:
    explicit base_64_helper();
    void encode(const std::string& encode, std::string& result) const noexcept;
    void decode(const std::string& decode, std::string& result) const noexcept;
	
private:
	class base_64_helper_impl;
	std::unique_ptr<base_64_helper_impl> impl;
}; // class

} // namespace

#endif // BASE64_HELPER_H
