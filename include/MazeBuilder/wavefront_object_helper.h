#ifndef WAVEFRONT_OBJECT_HELPER
#define WAVEFRONT_OBJECT_HELPER

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace mazes {

    class maze;

	class wavefront_object_helper {
    public:
        explicit wavefront_object_helper();
        void to_wavefront_object_str(const std::unique_ptr<maze>& m,
            const std::vector<std::tuple<int, int, int>>& vertices,
            const std::vector<std::vector<std::uint32_t>>& faces,
            std::string& result) const noexcept;
    private:
        class wavefront_object_helper_impl;
        std::unique_ptr<wavefront_object_helper_impl> impl;
	};

}

#endif // WAVEFRONT_OBJECT_HELPER
