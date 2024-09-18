#ifndef WRITER_H
#define WRITER_H

#include <vector>
#include <cstdint>
#include <string>

#include "output_types_enum.h"

namespace mazes {

class writer {
public:
	writer();
	output_types get_output_type(const std::string& filename) const noexcept;
	bool write(const std::string& filename, const std::string& data) const noexcept;
	bool write_png(const std::string& filename, const std::vector<std::uint8_t>& data, 
		const unsigned int w = 1, const unsigned int h = 1) const;
private:
	void write_file(const std::string& filename, const std::string& data) const;
}; // writer

}

#endif // WRITER_H