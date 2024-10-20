/**
 * Writer class handles stdout, and basic file writing to text or Wavefront obj file
 */

#include <MazeBuilder/writer.h>

#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cassert>

#include <MazeBuilder/output_types_enum.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

using namespace std;
using namespace mazes;

writer::writer() {

}

output_types writer::get_output_type(const std::string& filename) const noexcept {
	if (filename.compare("stdout") == 0) {
		return output_types::STDOUT;
	}

	// size of file extension is always 4: <dot>[txt|obj|png]
	static constexpr auto FILE_EXT_LEN = 4;
	auto found = filename.find(".");
	if (found == string::npos) {
		return output_types::UNKNOWN;
	}
	auto short_str = filename.substr(found, string::npos);
	if (short_str.length() == FILE_EXT_LEN && short_str == ".txt") {
		return output_types::PLAIN_TEXT;
	}
	else if (short_str.length() == FILE_EXT_LEN && short_str == ".obj") {
		return output_types::WAVEFRONT_OBJ_FILE;
	}
	else if (short_str.length() == FILE_EXT_LEN && short_str == ".png") {
		return output_types::PNG;
	}
	else {
		return output_types::UNKNOWN;
	}
}

/**
 * @brief writer::write
 * @param filename can be stdout, .txt, .png, or .obj
 * @param data represents a grid in plain text (ASCII) or Wavefront object file (mesh) or PNG (bytes)
 * @return
 */
bool writer::write(const std::string& filename, const std::string& data) const noexcept {
    auto otype = get_output_type(filename);

	// open file stream and start writing the data as per the file type
	try {
		if (otype == output_types::PLAIN_TEXT) {
			this->write_file(filename, data);
			return true;
		} else if (otype == output_types::WAVEFRONT_OBJ_FILE) {
			this->write_file(filename, data);
			return true;
		} else if (otype == output_types::STDOUT) {
			cout << data << endl;
			return true;
		} else if (otype == output_types::PNG) {
			cerr << "ERROR: Incorrect arguments for write_png with output: " << filename << "\n";
			return false;
		} else if (otype == output_types::UNKNOWN) {
			cerr << "ERROR: Unknown output type!!\n";
			return false;
		} else {
			cerr << "ERROR: Unknown output type!!\n";
			return false;
		}
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
bool writer::write_png(const std::string& filename, const std::vector<std::uint8_t>& data,
	const unsigned int w, const unsigned int h) const {
	return stbi_write_png(filename.c_str(), w, h, 4, data.data(), w * 4);
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
