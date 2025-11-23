# Maze Projection Usage Guide

## Problem Identified ✅ FIXED

**Issue:** The `extract_maze_walls()` function was incorrectly trying to use `objectify::run()` on an empty maze grid, which doesn't work because:
1. `objectify` expects a maze that's already been generated (with cell links)
2. We were passing a copy that had no links established
3. The API requires the maze to be fully constructed before extracting geometry

**Solution:** Rewrote `extract_maze_walls()` to:
1. Use `grid_operations::get()` to access cells directly
2. Check `cell->is_linked()` for each neighbor
3. Draw walls only where links DON'T exist
4. Handle empty cells gracefully

## How to Use Maze Projector

### Step 1: Generate a Maze First

```cpp
#include <MazeBuilder/create.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid.h>

// Configure the maze
mazes::configurator config;
config.rows(10)
      .columns(10)
      .levels(1)
      .algo_id(mazes::algo::DFS)  // or BINARY_TREE, SIDEWINDER, etc.
      .seed(12345);

// Create the maze grid and generate it
auto maze_grid = mazes::create(config);
```

### Step 2: Project Maze onto Voxel Surface

```cpp
#include "maze_projector.h"

// Initialize projector (do this once)
craft_rendering::MazeProjector projector;
projector.initialize();

// Configure projection
craft_rendering::MazeProjector::ProjectionConfig proj_config;
proj_config.target_x = 10;      // Voxel X position
proj_config.target_y = 5;       // Voxel Y position
proj_config.target_z = 8;       // Voxel Z position
proj_config.face = 5;           // 5 = Top face (0-5 for all faces)
proj_config.scale = 3.0f;       // Size of maze on surface
proj_config.rows = 10;          // Match maze dimensions
proj_config.columns = 10;

// Generate the projection geometry
auto geometry = projector.project_maze(maze_grid, proj_config);

// Check if valid
if (!geometry.is_valid) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to project maze\n");
    return;
}
```

### Step 3: Render the Projected Maze

```cpp
// Prepare view matrix (use existing craft matrix functions)
float view_matrix[16];
set_matrix_3d(view_matrix, width, height, cam_x, cam_y, cam_z,
              cam_rx, cam_ry, fov, is_ortho, render_radius);

// Enable blending for semi-transparent preview
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDisable(GL_DEPTH_TEST);  // Draw on top

// Render the maze geometry
projector.render_maze_geometry(geometry, view_matrix);

// Restore state
glDisable(GL_BLEND);
glEnable(GL_DEPTH_TEST);
```

## Face Orientation Reference

```
Face 0: -X (Left side)
Face 1: +X (Right side)
Face 2: -Z (Back)
Face 3: +Z (Front)
Face 4: -Y (Bottom)
Face 5: +Y (Top)
```

## Complete Integration Example

```cpp
// In craft.cpp, add to craft_impl:
craft_rendering::MazeProjector m_maze_projector;
std::optional<mazes::grid> m_preview_maze;
craft_rendering::MazeProjector::MazeGeometry m_preview_geometry;
bool m_show_maze_preview = false;
int m_preview_face = 5;  // Top face by default

// Initialize in run() method:
m_impl->m_maze_projector.initialize();

// When hover is detected (in mouse motion handler):
if (m_impl->m_show_maze_preview && hovered_voxel_valid) {
    // Generate maze if not already created
    if (!m_impl->m_preview_maze.has_value()) {
        mazes::configurator config;
        config.rows(gui->rows)
              .columns(gui->columns)
              .levels(1)
              .algo_id(mazes::algo::DFS)
              .seed(gui->seed);

        m_impl->m_preview_maze = mazes::create(config);
    }

    // Project onto hovered voxel
    craft_rendering::MazeProjector::ProjectionConfig proj_config;
    proj_config.target_x = hovered_x;
    proj_config.target_y = hovered_y;
    proj_config.target_z = hovered_z;
    proj_config.face = m_impl->m_preview_face;
    proj_config.scale = 3.0f;
    proj_config.rows = gui->rows;
    proj_config.columns = gui->columns;

    m_impl->m_preview_geometry =
        m_impl->m_maze_projector.project_maze(
            m_impl->m_preview_maze.value(), proj_config);

    // Render in the render loop
    if (m_impl->m_preview_geometry.is_valid) {
        float matrix[16];
        set_matrix_3d(matrix, ...);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_impl->m_maze_projector.render_maze_geometry(
            m_impl->m_preview_geometry, matrix);

        glDisable(GL_BLEND);
    }
}
```

## GUI Controls to Add

```cpp
// In ImGui settings:
ImGui::Checkbox("Show Maze Preview", &m_impl->m_show_maze_preview);

if (m_impl->m_show_maze_preview) {
    ImGui::SliderInt("Preview Face", &m_impl->m_preview_face, 0, 5);
    ImGui::Text("Face: %s", get_face_name(m_impl->m_preview_face));

    if (ImGui::Button("Regenerate Maze")) {
        m_impl->m_preview_maze.reset();  // Force regeneration
    }

    if (ImGui::Button("Build Maze Here")) {
        // TODO: Convert preview to actual voxels
        build_maze_from_preview();
    }
}
```

## Troubleshooting

### No walls appearing?
- **Check:** Is the maze actually generated? Call `create(config)` first
- **Check:** Are the maze dimensions > 0?
- **Check:** Is the scale factor reasonable (1.0 - 10.0)?

### Walls in wrong place?
- **Check:** Face orientation (0-5)
- **Check:** Target voxel coordinates
- **Check:** Scale factor - too small/large?

### Performance issues?
- **Cache geometry:** Don't regenerate every frame
- **Limit maze size:** Keep rows/columns < 50 for preview
- **Frustum culling:** Only generate when voxel is visible

## API Reference

### MazeProjector Methods

```cpp
bool initialize();  // Call once at startup
MazeGeometry project_maze(const mazes::grid& maze, const ProjectionConfig& config);
MazeGeometry generate_outline(const mazes::grid& maze, const ProjectionConfig& config);
void render_maze_geometry(const MazeGeometry& geometry, const float* matrix);
```

### ProjectionConfig Structure

```cpp
struct ProjectionConfig {
    int target_x, target_y, target_z;  // Voxel position
    int face;                           // Face direction (0-5)
    float scale;                        // Maze size on surface
    int rows, columns, height;          // Maze dimensions
};
```

### MazeGeometry Structure

```cpp
struct MazeGeometry {
    std::vector<float> vertices;        // Position data
    std::vector<float> colors;          // Color data (RGBA)
    std::vector<unsigned int> indices;  // Line indices
    gl::GlBuffer vbo_vertices, vbo_colors;
    gl::GlBuffer ebo;
    gl::GlVertexArray vao;
    size_t index_count;
    bool is_valid;
};
```

## What Was Fixed

1. ✅ **Removed objectify dependency** - Was causing empty geometry
2. ✅ **Direct cell link checking** - Uses `cell->is_linked()` properly
3. ✅ **Grid operations API** - Uses `grid_operations::get()` correctly
4. ✅ **Wall generation logic** - Draws walls where NO links exist
5. ✅ **Error handling** - Handles empty cells and invalid dimensions

## Build and Test

```powershell
# Rebuild with fixed maze projection
cd C:\Users\zachm\Desktop\zmertens-MazeBuilder\cmake-build-debug-visual-studio
cmake --build . --target mazebuildervoxels -j 14

# Run and test
.\bin\Voxels\mazebuildervoxels.exe
```


