#include <MazeBuilder/wavefront_object_helper.h>

#include <MazeBuilder/maze.h>

#include <sstream>
#include <cstdint>
#include <vector>

using namespace mazes;

class wavefront_object_helper::wavefront_object_helper_impl {
public:
    wavefront_object_helper_impl() = default;
    ~wavefront_object_helper_impl() = default;
    void to_wavefront_obj_str(const std::unique_ptr<maze>& m,
        const std::vector<std::tuple<int, int, int>>& vertices,
        const std::vector<std::vector<std::uint32_t>>& faces, std::string& result) const noexcept {
        using namespace std;

        stringstream ss;
        ss << "# https:www.github.com/zmertens/MazeBuilder\n";

        // Keep track of writing progress
        int total_verts = static_cast<int>(vertices.size());
        int total_faces = static_cast<int>(faces.size());

        int t = total_verts + total_faces;
        int c = 0;
        // Write vertices
        for (const auto& vertex : vertices) {
            float x = static_cast<float>(get<0>(vertex));
            float y = static_cast<float>(get<1>(vertex));
            float z = static_cast<float>(get<2>(vertex));
            ss << "v " << x << " " << y << " " << z << "\n";
            c++;
        }

        // Write faces
        for (const auto& face : faces) {
            ss << "f";
            for (auto index : face) {
                ss << " " << index;
            }
            ss << "\n";
            c++;
        }
        // Store the result
        result = ss.str();
    }
};  // wavefront_object_helper_impl

wavefront_object_helper::wavefront_object_helper() : impl{ std::make_unique<wavefront_object_helper_impl>() } {}

void wavefront_object_helper::to_wavefront_object_str(const std::unique_ptr<maze>& m,
    const std::vector<std::tuple<int, int, int>>& vertices,
    const std::vector<std::vector<std::uint32_t>>& faces,
    std::string& result) const noexcept {
    this->impl->to_wavefront_obj_str(cref(m), cref(vertices), cref(faces), ref(result));
} // to_wavefront_obj_str
