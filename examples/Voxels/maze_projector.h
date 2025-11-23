#ifndef MAZE_PROJECTOR_H
#define MAZE_PROJECTOR_H

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include "gl_resource_manager.h"
#include <MazeBuilder/maze_interface.h>
#include <MazeBuilder/grid.h>
#include <vector>
#include <memory>

namespace craft_rendering {

/// @brief Projects maze data onto voxel surfaces with isometric view
class MazeProjector {
public:
    struct ProjectionConfig {
        int target_x{0};           // Voxel position
        int target_y{0};
        int target_z{0};
        int face{0};               // Face direction (0-5)
        float scale{1.0f};         // Maze scale on surface
        int rows{10};              // Maze dimensions
        int columns{10};
        int height{1};
    };

    struct MazeGeometry {
        std::vector<float> vertices;     // Position data
        std::vector<float> colors;       // Color data for walls/paths
        std::vector<unsigned int> indices;
        gl::GlBuffer vbo_vertices;
        gl::GlBuffer vbo_colors;
        gl::GlBuffer ebo;
        gl::GlVertexArray vao;
        size_t index_count{0};
        bool is_valid{false};
    };

    MazeProjector();
    ~MazeProjector() = default;

    /// @brief Initialize projector
    bool initialize();

    /// @brief Generate maze geometry projected onto a voxel face
    /// @param maze_data The maze grid data
    /// @param config Projection configuration
    /// @return Generated geometry ready for rendering
    MazeGeometry project_maze(const mazes::grid& maze_data, const ProjectionConfig& config);

    /// @brief Generate maze outline for preview
    /// @param maze_data The maze grid data
    /// @param config Projection configuration
    /// @return Outline geometry
    MazeGeometry generate_outline(const mazes::grid& maze_data, const ProjectionConfig& config);

    /// @brief Render projected maze geometry
    /// @param geometry The maze geometry to render
    /// @param matrix View-projection matrix
    void render_maze_geometry(const MazeGeometry& geometry, const float* matrix);

    /// @brief Check if initialized
    bool is_initialized() const { return m_initialized; }

private:
    gl::GlShaderProgram m_maze_shader;
    bool m_initialized{false};

    /// @brief Calculate projection matrix for face
    void calculate_face_transform(int face, float x, float y, float z, float scale, float* transform);

    /// @brief Extract wall/path data from maze
    void extract_maze_walls(const mazes::grid& maze_data,
                           std::vector<float>& vertices,
                           std::vector<unsigned int>& indices);
};

} // namespace craft_rendering

#endif // MAZE_PROJECTOR_H

