#ifndef WAVEFRONT_OBJECT_HELPER
#define WAVEFRONT_OBJECT_HELPER

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace mazes {

class maze;

/// @file wavefront_object_helper.h

/// @class wavefront_object_helper
/// @brief Transform a maze into a Wavefront object string
class wavefront_object_helper {
public:
    // Default constructor
    explicit wavefront_object_helper();

    // Destructor
    ~wavefront_object_helper();

    // Copy constructor
    wavefront_object_helper(const wavefront_object_helper& other);

    // Copy assignment operator
    wavefront_object_helper& operator=(const wavefront_object_helper& other);

    // Move constructor
    wavefront_object_helper(wavefront_object_helper&& other) noexcept = default;

    // Move assignment operator
    wavefront_object_helper& operator=(wavefront_object_helper&& other) noexcept = default;

    /// @brief Transform a maze into a Wavefront object string
    /// @details The Wavefront object string can be used as an import file for game engines
    /// @param m
    /// @param vertices 
    /// @param faces 
    /// @return 
    std::string to_wavefront_object_str(const std::unique_ptr<maze>& m,
        const std::vector<std::tuple<int, int, int, int>>& vertices,
        const std::vector<std::vector<std::uint32_t>>& faces) const noexcept;
private:
    /// @brief Forward declaration of the implementation class
    class wavefront_object_helper_impl;
    std::unique_ptr<wavefront_object_helper_impl> impl;
};

}

#endif // WAVEFRONT_OBJECT_HELPER
