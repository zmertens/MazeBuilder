#ifndef CRAFT_TYPES_H
#define CRAFT_TYPES_H

#include "gl_types.h"

#include <memory>
#include <string>

#include "map.h"
#include "sign.h"

// Basic geometry and state structures
struct CraftBlock {
    int x, y, z, w;
};

struct CraftState {
    float x, y, z;
    float rx, ry;
    float t;
};

struct CraftPlayer {
    int id;
    std::string name;
    CraftState state;
    CraftState state1;
    CraftState state2;
    gl_buffer_handle buffer;
};

struct CraftChunk {
    Map map;
    Map lights;
    SignList signs;
    int p, q;
    int faces, sign_faces;
    int dirty;
    int miny, maxy;
    gl_buffer_handle buffer;
    gl_buffer_handle sign_buffer;
};

struct CraftAttrib {
    gl_program_handle program;
    gl_unsigned_int position;
    gl_unsigned_int normal;
    gl_unsigned_int uv;
    gl_int matrix;
    gl_int sampler;
    gl_int camera;
    gl_int timer;
    gl_int extra1;
    gl_int extra2;
    gl_int extra3;
    gl_int extra4;
};

struct RenderParams {
    int scene_w, scene_h;
    float camera_x, camera_y, camera_z;
    float rotation_x, rotation_y;
    float fov;
    bool is_ortho;
    int render_radius;
    int scale;
};

#endif // CRAFT_TYPES_H
