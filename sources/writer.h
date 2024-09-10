#ifndef WRITER_H
#define WRITER_H

#include <string>

#include "file_types_enum.h"

namespace mazes {

class writer {
public:
	writer();
	file_types get_filetype(const std::string& filename) const noexcept;
	bool write(const std::string& filename, const std::string& data, const unsigned int w = 1, const unsigned int h = 1) const;
private:
	void write_file(const std::string& filename, const std::string& data) const;
	void write_png(const std::string& filename, const std::string& data, const unsigned int w = 1, const unsigned int h = 1) const;
}; // writer

}

#endif // WRITER_H