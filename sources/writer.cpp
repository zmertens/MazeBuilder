/**
 * Writer class handles stdout, and basic file writing to text or Wavefront obj file
 */

#include "writer.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cassert>

#include "file_types_enum.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

using namespace std;
using namespace mazes;

writer::writer() {

}

file_types writer::get_filetype(const std::string& filename) const noexcept {
	// size of file extension is always 4: <dot>[txt|obj|png]
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
	else if (short_str.length() == FILE_EXT_LEN && short_str == ".png") {
		return file_types::PNG;
	}
	else {
		return file_types::UNKNOWN;
	}
}

/**
 * @brief writer::write
 * @param filename can be stdout, .txt, or .obj
 * @param data represents a grid in plain text (ASCII) or Wavefront object file (mesh) or PNG (bytes)
 * @return
 */
bool writer::write(const std::string& filename, const std::string& data) const {

    if (filename.compare("stdout") == 0) {
        cout << data << endl;
        return true;
    }

    auto ftype = get_filetype(filename);
	if (!filename.empty()) {
		// keep writing
	} else {
		// indeterminable file type
		throw new runtime_error("Unknown file type for filename: " + filename);
	}

	// open file stream and start writing the data as per the file type
	try {
		if (ftype == file_types::PLAIN_TEXT) {
			this->write_file(filename, data);
		} else if (ftype == file_types::WAVEFRONT_OBJ_FILE) {
			this->write_file(filename, data);
		} else {
			throw new runtime_error("Unknown file type for filename: " + filename);
		}
		return true;
	} catch (...) {
		return false;
	}
}

/**
 * @brief writer::write_file
 * @param filename
 * @param data
 * @param w = 1
 * @param h = 1
 */
bool writer::write_png(const std::string& filename, const std::vector<std::uint8_t>& data, const unsigned int w, const unsigned int h) const {
	static constexpr auto CELL_SIZE = 25;
	return stbi_write_png(filename.c_str(), w * CELL_SIZE + 1, h * CELL_SIZE + 1, 4, data.data(), (w * CELL_SIZE + 1) * 4);
}

/**
 * @brief writer::write_file
 * @param filename
 * @param data
 */
void writer::write_file(const std::string& filename, const std::string& data) const {
    filesystem::path data_path {filename};
    ofstream out_writer {data_path};
	out_writer << data;
	out_writer.close();
}
