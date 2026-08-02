#ifndef GEOMETRIES_H
#define GEOMETRIES_H

#include <cstdint>
#include <optional>
#include <vector>

namespace mazes
{
    class configurator;
}

struct maze_preview_frame
{
    std::vector<std::uint8_t> pixel_data;
    int width{0};
    int height{0};
    int scale{1};
};

class geometries
{
public:
    static void make_cube_faces(
        float *data, float ao[6][4], float light[6][4],
        int left, int right, int top, int bottom, int front, int back,
        int wleft, int wright, int wtop, int wbottom, int wfront, int wback,
        float x, float y, float z, float n) noexcept;

    static void make_cube(
        float *data, float ao[6][4], float light[6][4],
        int left, int right, int top, int bottom, int front, int back,
        float x, float y, float z, float n, int w) noexcept;

    static void make_plant(
        float *data, float ao, float light,
        float px, float py, float pz, float n, int w, float rotation) noexcept;

    static void make_player(
        float *data,
        float x, float y, float z, float rx, float ry) noexcept;

    static void make_cube_wireframe(
        float *data, float x, float y, float z, float n) noexcept;

    static void make_character(
        float *data,
        float x, float y, float n, float m, char c) noexcept;

    static void make_character_3d(
        float *data, float x, float y, float z, float n, int face, char c) noexcept;

    static void make_sphere(float *data, float r, int detail) noexcept;

    static std::optional<maze_preview_frame> generate_maze_preview(
        const mazes::configurator &config);

private:
    static int _make_sphere(
        float *data, float r, int detail,
        float *a, float *b, float *c,
        float *ta, float *tb, float *tc) noexcept;
};

#endif // GEOMETRIES_H
