#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <string>

namespace mazes {

    /// @brief Holds basic I/O information and enums
    /// @brief Enumerates the output types
    enum class outputs : unsigned int {
        STDOUT = 0,
        PLAIN_TEXT = 1,
        WAVEFRONT_OBJ = 2,
        PNG = 3,
        JPEG = 4,
        UNKNOWN = 5
    };

    class writer {

    };

    /// @brief Determine the output type based on the filename
    /// @param filename 
    /// @return outputs 
    outputs get_output(const std::string& filename) noexcept {
        if (filename.empty()) {
            return outputs::STDOUT;
        }

        if (filename.find(".obj") != std::string::npos) {
            return outputs::WAVEFRONT_OBJ;
        }

        if (filename.find(".png") != std::string::npos) {
            return outputs::PNG;
        }

        if (filename.find(".jpg") != std::string::npos) {
            return outputs::JPEG;
        }

        return outputs::PLAIN_TEXT;
    }
}

#endif // OUTPUTS_H
