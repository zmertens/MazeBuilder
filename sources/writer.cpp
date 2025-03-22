#include <MazeBuilder/writer.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <MazeBuilder/enums.h>
#include <MazeBuilder/json_helper.h>

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

    if (!out_writer.is_open()) {
        return false;
    }

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

    // Handle the output type
	try {
        if (filename == "stdout") {
            throw invalid_argument("Invalid use of stdout.");
        }

        auto ftype = writer::is_file_with_suffix(cref(filename));

		if (writer::is_file_with_suffix(cref(filename), output::PLAIN_TEXT)) {
			return this->write_file(filename, data);
        } else if (writer::is_file_with_suffix(cref(filename), output::JSON)) {
            return this->write_file(cref(filename), data);
        } else if (writer::is_file_with_suffix(cref(filename), output::WAVEFRONT_OBJECT_FILE)) {
			return this->write_file(filename, data);
		} else if (writer::is_file_with_suffix(cref(filename), output::PNG)) {
            return this->write_png(filename, data);
		} else if (writer::is_file_with_suffix(cref(filename), output::JPEG)) {
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

/// @brief Verify the file type by the output type provided
/// @param f the file to verify
/// @param o output::PLAIN_TEXT
bool writer::is_file_with_suffix(const std::string& f, output o) noexcept {
    using namespace std;

    // Verify file has valid extension
    auto found = f.find(".");
    if (found == string::npos) {
        return false;
    }

    static constexpr auto MAX_FILE_EXT_LEN = 7;

    // Determine file type by the extension
    auto short_str = f.substr(found, string::npos);

    if (short_str == ".txt" && o == output::PLAIN_TEXT) {
        return true;
    } else if (short_str == ".text" && o == output::PLAIN_TEXT) {
        return true;
    } else if (short_str == ".obj" && o == output::WAVEFRONT_OBJECT_FILE) {
        return true;
    } else if (short_str == ".object" && o == output::WAVEFRONT_OBJECT_FILE) {
        return true;
    } else if (short_str == ".png" && o == output::PNG) {
        return true;
    } else if (short_str == ".jpeg" && o == output::JPEG) {
        return true;
    } else if (short_str == ".jpg" && o == output::JPEG) {
        return true;
    } else if (short_str == ".json" && o == output::JSON) {
        return true;
    } else {
        return false;
    }
}
