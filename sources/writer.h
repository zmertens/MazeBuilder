#ifndef WRITER_H
#define WRITER_H

#include <string>

#include "file_types_enum.h"

namespace mazes {

class writer {
public:
	writer();
	file_types get_filetype(const std::string& filename) const noexcept;
	bool write(const std::string& filename, const std::string& data) const;
private:
	void write_wavefront_obj(const std::string& filename, const std::string& data) const;
	void write_plain_text(const std::string& filename, const std::string& data) const;
}; // writer

}

#endif // WRITER_H