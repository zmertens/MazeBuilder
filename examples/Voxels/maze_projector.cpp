#include "maze_projector.h"
#include "matrix.h"
#include <MazeBuilder/cell.h>
#include <MazeBuilder/create.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/objectify.h>
#include <MazeBuilder/grid_operations.h>
#include <SDL3/SDL.h>
#include <cmath>

namespace craft_rendering {

MazeProjector::MazeProjector() = default;

bool MazeProjector::initialize() {
#if defined(__EMSCRIPTEN__)
    const char* vertex_path = "shaders/es/maze_vertex.es.glsl";
    const char* fragment_path = "shaders/es/maze_fragment.es.glsl";
#else
    const char* vertex_path = "shaders/maze_vertex.glsl";
    const char* fragment_path = "shaders/maze_fragment.glsl";
#endif

    if (!m_maze_shader.load_from_files(vertex_path, fragment_path)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load maze projection shaders\n");
        return false;
    }

    m_initialized = true;
    SDL_Log("MazeProjector: Initialized successfully\n");
    return true;
}

MazeProjector::MazeGeometry MazeProjector::project_maze(const mazes::grid& maze_data,
                                                         const ProjectionConfig& config) {
    MazeGeometry geometry;

    // Extract maze wall/path data
    extract_maze_walls(maze_data, geometry.vertices, geometry.indices);

    if (geometry.vertices.empty()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "MazeProjector: No maze geometry generated\n");
        return geometry;
    }

    // Calculate transformation for the target face
    float transform[16];
    calculate_face_transform(config.face,
                            static_cast<float>(config.target_x),
                            static_cast<float>(config.target_y),
                            static_cast<float>(config.target_z),
                            config.scale, transform);

    // Apply transformation to vertices
    for (size_t i = 0; i < geometry.vertices.size(); i += 3) {
        float x = geometry.vertices[i];
        float y = geometry.vertices[i + 1];
        float z = geometry.vertices[i + 2];

        // Apply transformation
        float tx = transform[0] * x + transform[4] * y + transform[8] * z + transform[12];
        float ty = transform[1] * x + transform[5] * y + transform[9] * z + transform[13];
        float tz = transform[2] * x + transform[6] * y + transform[10] * z + transform[14];

        geometry.vertices[i] = tx;
        geometry.vertices[i + 1] = ty;
        geometry.vertices[i + 2] = tz;
    }

    // Generate colors (walls = white, paths = transparent)
    geometry.colors.resize(geometry.vertices.size() / 3 * 4); // RGBA per vertex
    for (size_t i = 0; i < geometry.colors.size(); i += 4) {
        geometry.colors[i] = 1.0f;     // R
        geometry.colors[i + 1] = 0.8f; // G
        geometry.colors[i + 2] = 0.2f; // B (orange/yellow preview)
        geometry.colors[i + 3] = 0.7f; // A (semi-transparent)
    }

    // Create OpenGL buffers
    geometry.vao.bind();

    // Vertex positions
    geometry.vbo_vertices.bind(GL_ARRAY_BUFFER);
    geometry.vbo_vertices.allocate_data(GL_ARRAY_BUFFER,
                                       geometry.vertices.size() * sizeof(float),
                                       geometry.vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    // Vertex colors
    geometry.vbo_colors.bind(GL_ARRAY_BUFFER);
    geometry.vbo_colors.allocate_data(GL_ARRAY_BUFFER,
                                     geometry.colors.size() * sizeof(float),
                                     geometry.colors.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

    // Indices
    geometry.ebo.bind(GL_ELEMENT_ARRAY_BUFFER);
    geometry.ebo.allocate_data(GL_ELEMENT_ARRAY_BUFFER,
                              geometry.indices.size() * sizeof(unsigned int),
                              geometry.indices.data(), GL_STATIC_DRAW);

    geometry.vao.unbind();

    geometry.index_count = geometry.indices.size();
    geometry.is_valid = true;

    return geometry;
}

MazeProjector::MazeGeometry MazeProjector::generate_outline(const mazes::grid& maze_data,
                                                             const ProjectionConfig& config) {
    // For outline, generate just the perimeter
    MazeGeometry geometry;

    auto [rows, columns, levels] = maze_data.get_dimensions();

    float cell_size = config.scale / static_cast<float>(std::max(rows, columns));
    float offset_x = -config.scale * 0.5f;
    float offset_z = -config.scale * 0.5f;

    // Generate outline box
    std::vector<float> outline_verts = {
        offset_x, 0.0f, offset_z,
        offset_x + config.scale, 0.0f, offset_z,
        offset_x + config.scale, 0.0f, offset_z + config.scale,
        offset_x, 0.0f, offset_z + config.scale
    };

    std::vector<unsigned int> outline_indices = {
        0, 1, 1, 2, 2, 3, 3, 0
    };

    geometry.vertices = outline_verts;
    geometry.indices = outline_indices;

    // Calculate transformation for the target face
    float transform[16];
    calculate_face_transform(config.face,
                            static_cast<float>(config.target_x),
                            static_cast<float>(config.target_y),
                            static_cast<float>(config.target_z),
                            1.0f, transform);

    // Apply transformation
    for (size_t i = 0; i < geometry.vertices.size(); i += 3) {
        float x = geometry.vertices[i];
        float y = geometry.vertices[i + 1];
        float z = geometry.vertices[i + 2];

        float tx = transform[0] * x + transform[4] * y + transform[8] * z + transform[12];
        float ty = transform[1] * x + transform[5] * y + transform[9] * z + transform[13];
        float tz = transform[2] * x + transform[6] * y + transform[10] * z + transform[14];

        geometry.vertices[i] = tx;
        geometry.vertices[i + 1] = ty;
        geometry.vertices[i + 2] = tz;
    }

    // Setup buffers
    geometry.vao.bind();

    geometry.vbo_vertices.bind(GL_ARRAY_BUFFER);
    geometry.vbo_vertices.allocate_data(GL_ARRAY_BUFFER,
                                       geometry.vertices.size() * sizeof(float),
                                       geometry.vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    geometry.ebo.bind(GL_ELEMENT_ARRAY_BUFFER);
    geometry.ebo.allocate_data(GL_ELEMENT_ARRAY_BUFFER,
                              geometry.indices.size() * sizeof(unsigned int),
                              geometry.indices.data(), GL_STATIC_DRAW);

    geometry.vao.unbind();

    geometry.index_count = geometry.indices.size();
    geometry.is_valid = true;

    return geometry;
}

void MazeProjector::render_maze_geometry(const MazeGeometry& geometry, const float* matrix) {
    if (!geometry.is_valid) {
        return;
    }

    m_maze_shader.use();
    m_maze_shader.set_uniform_matrix4fv("matrix", matrix);

    geometry.vao.bind();
    glDrawElements(GL_LINES, static_cast<GLsizei>(geometry.index_count), GL_UNSIGNED_INT, nullptr);
    geometry.vao.unbind();
}

void MazeProjector::calculate_face_transform(int face, float x, float y, float z,
                                             float scale, float* transform) {
    // Initialize as identity matrix
    for (int i = 0; i < 16; ++i) {
        transform[i] = 0.0f;
    }
    transform[0] = transform[5] = transform[10] = transform[15] = 1.0f;

    // Translation to voxel center
    transform[12] = x + 0.5f;
    transform[13] = y + 0.5f;
    transform[14] = z + 0.5f;

    // Apply face-specific rotation and translation
    // Face 0: -X (left)
    // Face 1: +X (right)
    // Face 2: -Z (back)
    // Face 3: +Z (front)
    // Face 4: -Y (bottom)
    // Face 5: +Y (top)

    switch (face) {
    case 0: // -X face
        transform[0] = 0.0f;  transform[8] = -scale;
        transform[2] = scale;  transform[10] = 0.0f;
        transform[12] = x;
        break;
    case 1: // +X face
        transform[0] = 0.0f;  transform[8] = scale;
        transform[2] = scale;  transform[10] = 0.0f;
        transform[12] = x + 1.0f;
        break;
    case 2: // -Z face
        transform[0] = scale; transform[8] = 0.0f;
        transform[14] = z;
        break;
    case 3: // +Z face
        transform[0] = scale; transform[8] = 0.0f;
        transform[14] = z + 1.0f;
        break;
    case 4: // -Y face
        transform[5] = 0.0f;  transform[9] = -scale;
        transform[6] = scale; transform[10] = 0.0f;
        transform[13] = y;
        break;
    case 5: // +Y face (top)
        transform[5] = 0.0f;  transform[9] = scale;
        transform[6] = scale; transform[10] = 0.0f;
        transform[13] = y + 1.0f;
        break;
    }
}

void MazeProjector::extract_maze_walls(const mazes::grid& maze_data,
                                       std::vector<float>& vertices,
                                       std::vector<unsigned int>& indices) {
    // Use objectify to generate 3D mesh data from maze
    mazes::objectify obj_generator;
    mazes::randomizer rng; // Unused but required by API

    // Create a mutable copy to work with objectify
    mazes::grid maze_copy = maze_data;

    // Generate vertices and faces using objectify
    if (!obj_generator.run(&maze_copy, rng)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to generate maze geometry\n");
        return;
    }

    // Get the generated vertices and faces from grid operations
    const auto& grid_ops = maze_copy.operations();
    const auto& obj_vertices = grid_ops.get_vertices();
    const auto& obj_faces = grid_ops.get_faces();

    if (obj_vertices.empty() || obj_faces.empty()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No geometry generated from maze\n");
        return;
    }

    // Convert vertices to float array (normalize coordinates)
    auto [rows, columns, levels] = maze_data.get_dimensions();
    float scale = 1.0f / static_cast<float>(std::max(rows, columns));
    float offset_x = -0.5f;
    float offset_y = -0.5f;
    float offset_z = -0.5f;

    for (const auto& vert : obj_vertices) {
        // Extract x, y, z from tuple (w is ignored for now)
        auto [x, y, z, w] = vert;

        // Normalize and center coordinates
        float nx = offset_x + x * scale;
        float ny = offset_y + y * scale;
        float nz = offset_z + z * scale;

        vertices.push_back(nx);
        vertices.push_back(ny);
        vertices.push_back(nz);
    }

    // Convert faces to indices (flatten triangles)
    for (const auto& face : obj_faces) {
        for (const auto& idx : face) {
            // OBJ format is 1-indexed, convert to 0-indexed
            indices.push_back(idx - 1);
        }
    }
}

} // namespace craft_rendering

