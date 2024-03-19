#include "writer.h"

#include <string_view>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "file_types_enum.h"

using namespace std;
using namespace mazes;

writer::writer() {

}

file_types writer::get_filetype(const std::string& filename) const noexcept {
	// size of file extension is always 4: <dot>[txt|obj]
	static constexpr auto FILE_EXT_LEN = 4;
	auto found = filename.find(".");
	if (found == string::npos) {
		return file_types::UNKNOWN;
	}
	auto short_str = filename.substr(found, string::npos);
	if (short_str.length() == FILE_EXT_LEN && short_str == ".txt") {
		return file_types::PLAIN_TEXT;
	}
	else if (short_str.length() == FILE_EXT_LEN && short_str == ".obj") {
		return file_types::WAVEFRONT_OBJ_FILE;
	}
	else {
		return file_types::UNKNOWN;
	}
}

bool writer::write(const std::string& filename, const std::string& data) const {
	bool success = false;
	file_types ftype = file_types::PLAIN_TEXT;
	if (!filename.empty()) {
		ftype = get_filetype(filename);
	} else {
		// indeterminable file type or stdout
		throw new runtime_error("Unknown file type for filename: " + filename);
	}

	// open file stream and start writing the data as per the file type
	if (ftype == file_types::PLAIN_TEXT) {
		this->write_plain_text(filename, data);
	} else {
		this->write_wavefront_obj(filename, data);
	}
	return success;
}

void writer::write_wavefront_obj(const std::string& filename, const std::string& data) const {
	string_view data_view{ data };
	filesystem::path data_path{ filename };
	ofstream out_writer{ data_path };
	
	for (auto&& obj_data : data_view) {
#if defined(DEBUGGING)
		out_writer << obj_data;
#endif
	}
	out_writer.close();
}

void writer::write_plain_text(const std::string& filename, const std::string& data) const {
	filesystem::path data_path{ filename };
	ofstream out_writer{ data_path };
	out_writer << data;
	out_writer.close();
}