#ifndef WRITER_H
#define WRITER_H

#include <vector>
#include <cstdlib>
#include <string>

#include "file_types_enum.h"

namespace mazes {

class writer {
public:
	writer();
	file_types get_filetype(const std::string& filename) const noexcept;
	bool write(const std::string& filename, const std::string& data) const;
	bool write_png(const std::string& filename, const std::vector<std::uint8_t>& data, 
		const unsigned int w = 1, const unsigned int h = 1, const unsigned int cell_size = 25) const;
private:
	void write_file(const std::string& filename, const std::string& data) const;
}; // writer

}

#endif // WRITER_H