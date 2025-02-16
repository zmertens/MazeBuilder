#ifndef WRITER_H
#define WRITER_H

#include <string>
#include <memory>

namespace mazes {

/// @brief Handles file writing for mazes
class writer {
public:
	explicit writer();
    ~writer();

	/// @brief Handles writing to a file
	/// @param filename 
	/// @param data
    /// @param w 1 for image files
    /// @param h 1 for image files
	/// @return 
	bool write(const std::string& filename, const std::string& data, unsigned int w = 1, unsigned int h = 1) const noexcept;

    /// @brief Handles writing to cout
    /// @param oss 
    /// @param data 
    /// @return 
    bool write(std::ostream& oss, const std::string& data) const noexcept;

private:
    class writer_impl;
    std::unique_ptr<writer_impl> m_impl;
}; // writer

}

#endif // WRITER_H
