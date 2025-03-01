#include <MazeBuilder/writer.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <MazeBuilder/enums.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

using namespace mazes;

/// @brief 
/// @param filename 
/// @param data 
/// @param w 1 
/// @param h 1
/// @return 
bool writer::write_png(const std::string& filename, const std::string& data, unsigned int w, unsigned int h) const noexcept {

    static constexpr auto STRIDE = 4u;

    return (0 != stbi_write_png(filename.c_str(), w, h, STRIDE, data.data(), w * STRIDE));
}

/// @brief 
/// @param filename 
/// @param data 
/// @param w 1
/// @param h 1
/// @return 
bool writer::write_jpeg(const std::string& filename, const std::string& data, unsigned int w, unsigned int h) const noexcept {

    static constexpr auto STRIDE = 4u;

    return (0 != stbi_write_jpg(filename.c_str(), w, h, STRIDE, data.data(), w * STRIDE));
}

/// @brief Write to a conventional file
/// @param filename 
/// @param data 
/// @return 
bool writer::write_file(const std::string& filename, const std::string& data) const noexcept {
    using namespace std;

    filesystem::path data_path{ filename };

    ofstream out_writer{ data_path };

    out_writer << data;

    out_writer.close();

    return out_writer.good();
}

/// @brief Main function for handling file I/O
/// @param filename 
/// @param data
/// @param w 1
/// @param h 1
/// @return 
bool writer::write(const std::string& filename, const std::string& data, unsigned int w, unsigned int h) const noexcept {
    using namespace std;

    static auto det_file_by_suffix = [](const string& f)->outputs {

        // Verify file has valid extension
        auto found = f.find(".");
        if (found == string::npos) {
            throw invalid_argument("Invalid filename: " + f);
        }

        static constexpr auto MAX_FILE_EXT_LEN = 7;

        // Determine file type by the extension
        auto short_str = f.substr(found, string::npos);

        if (short_str.length() <= MAX_FILE_EXT_LEN && short_str == ".txt") {
            return outputs::PLAIN_TEXT;
        } else if (short_str.length() <= MAX_FILE_EXT_LEN && short_str == ".text") {
            return outputs::PLAIN_TEXT;
        } else if (short_str.length() <= MAX_FILE_EXT_LEN && short_str == ".obj") {
            return outputs::WAVEFRONT_OBJECT_FILE;
        } else if (short_str.length() <= MAX_FILE_EXT_LEN && short_str == ".object") {
            return outputs::WAVEFRONT_OBJECT_FILE;
        } else if (short_str.length() <= MAX_FILE_EXT_LEN && short_str == ".png") {
            return outputs::PNG;
        } else if (short_str.length() <= MAX_FILE_EXT_LEN && short_str == ".jpeg") {
            return outputs::JPEG;
        } else if (short_str.length() <= MAX_FILE_EXT_LEN && short_str == ".jpg") {
            return outputs::JPEG;
        } else {
            throw invalid_argument("Invalid filename: " + f);
        }

        }; // lambda


    // Handle the output type
	try {
        if (filename == "stdout") {
            throw invalid_argument("Invalid use of stdout.");
        }

        auto ftype = det_file_by_suffix(cref(filename));

		if (ftype == outputs::PLAIN_TEXT) {
			return this->write_file(filename, data);
		} else if (ftype == outputs::WAVEFRONT_OBJECT_FILE) {
			return this->write_file(filename, data);
		} else if (ftype == outputs::PNG) {
            return this->write_png(filename, data);
		} else if (ftype == outputs::JPEG) {
            return this->write_jpeg(filename, data);
		} else {
			return false;
		}
	} catch (...) {
		return false;
	}
}

bool writer::write(std::ostream& oss, const std::string& data) const noexcept {
    using namespace std;

    oss << data << "\n";

    return oss.good();
}
