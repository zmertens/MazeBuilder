#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <functional>
#include <memory>
#include <list>

#include <glad/glad.h>

#include <SDL3/SDL.h>

#include <tinycthread/tinycthread.h>

#include "sign.h"
#include "map.h"
#include "config.h"

#include "maze_algo_interface.h"
#include "maze_types_enum.h"

#define MAX_CHUNKS 8192
#define MAX_PLAYERS 128
#define WORKERS 4
#define MAX_TEXT_LENGTH 256
#define MAX_NAME_LENGTH 32
#define MAX_PATH_LENGTH 256
#define MAX_ADDR_LENGTH 256

class grid;
struct Attrib;
struct Player;
struct Block;
struct Chunk;
struct WorkerItem;
struct Worker;

class craft : public mazes::maze_algo_interface {
public:
    craft(const std::string& window_name, mazes::maze_types maze_type);
    ~craft();
    bool run(mazes::grid& gr, std::function<int(int, int)> const& get_int, bool interactive = false) noexcept override;
    // std::list<std::unique_ptr<mazes::grid>> get_grids() const noexcept;
private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;
    // static std::unique_ptr<Model> g;


    // const std::string& m_window_name;
    // mazes::maze_types m_maze_type;
    // std::list<std::unique_ptr<mazes::grid>> m_grids;

    // int chunked(float x) const;
    // double get_time() const;
    // float time_of_day() const;
    // float get_daylight() const;
    // int get_scale_factor() const;
    // void get_sight_vector(float rx, float ry, float *vx, float *vy, float *vz) const;
    // void get_motion_vector(int flying, int sz, int sx, float rx, float ry, float *vx, float *vy, float *vz) const;
    // GLuint gen_crosshair_buffer();
    // GLuint gen_wireframe_buffer(float x, float y, float z, float n);
    // GLuint gen_sky_buffer();
    // GLuint gen_cube_buffer(float x, float y, float z, float n, int w);
    // GLuint gen_plant_buffer(float x, float y, float z, float n, int w);
    // GLuint gen_player_buffer(float x, float y, float z, float rx, float ry);
    // GLuint gen_text_buffer(float x, float y, float n, char *text);
    // void draw_triangles_3d_ao(Attrib *attrib, GLuint buffer, int count);
    // void draw_triangles_3d_text(Attrib *attrib, GLuint buffer, int count);
    // void draw_triangles_3d(Attrib *attrib, GLuint buffer, int count);
    // void draw_triangles_2d(Attrib *attrib, GLuint buffer, int count);
    // void draw_lines(Attrib *attrib, GLuint buffer, int components, int count);
    // void draw_chunk(Attrib *attrib, Chunk *chunk);
    // void draw_item(Attrib *attrib, GLuint buffer, int count);
    // void draw_text(Attrib *attrib, GLuint buffer, int length);
    // void draw_signs(Attrib *attrib, Chunk *chunk);
    // void draw_sign(Attrib *attrib, GLuint buffer, int length);
    // void draw_cube(Attrib *attrib, GLuint buffer);
    // void draw_plant(Attrib *attrib, GLuint buffer);
    // void draw_player(Attrib *attrib, Player *player);
    // Player *find_player(int id);
    // void update_player(Player *player, float x, float y, float z, float rx, float ry, int interpolate);
    // void interpolate_player(Player *player);
    // void delete_player(int id);
    // void delete_all_players();
    // float player_player_distance(Player *p1, Player *p2);
    // float player_crosshair_distance(Player *p1, Player *p2);
    // Player *player_crosshair(Player *player);
    // Chunk *find_chunk(int p, int q);
    // int chunk_distance(Chunk *chunk, int p, int q);
    // int chunk_visible(float planes[6][4], int p, int q, int miny, int maxy);
    // int highest_block(float x, float z);
    // int _hit_test(Map *map, float max_distance, int previous, float x, float y, float z, float vx, float vy, float vz, int *hx, int *hy, int *hz);
    // int hit_test(int previous, float x, float y, float z, float rx, float ry, int *bx, int *by, int *bz);
    // int hit_test_face(Player *player, int *x, int *y, int *z, int *face);
    // int collide(int height, float *x, float *y, float *z);
    // int player_intersects_block(int height, float x, float y, float z, int hx, int hy, int hz);
    // int _gen_sign_buffer(GLfloat *data, float x, float y, float z, int face, const char *text);
    // void gen_sign_buffer(Chunk *chunk);
    // int has_lights(Chunk *chunk);
    // void dirty_chunk(Chunk *chunk);
    // void occlusion(char neighbors[27], char lights[27], float shades[27], float ao[6][4], float light[6][4]);
    // void light_fill(char *opaque, char *light, int x, int y, int z, int w, int force);
    // void compute_chunk(WorkerItem *item);
    // void generate_chunk(Chunk *chunk, WorkerItem *item);
    // void gen_chunk_buffer(Chunk *chunk);
    
    // void load_chunk(WorkerItem *item);
    // void init_chunk(Chunk *chunk, int p, int q);
    // void create_chunk(Chunk *chunk, int p, int q);
    // void delete_chunks();
    // void delete_all_chunks();
    // void check_workers();
    // void force_chunks(Player *player);
    // void ensure_chunks_worker(Player *player, Worker *worker);
    // void ensure_chunks(Player *player);
    // static int worker_run(void *arg);
    // void unset_sign(int x, int y, int z);
    // void unset_sign_face(int x, int y, int z, int face);
    // void _set_sign(int p, int q, int x, int y, int z, int face, const char *text, int dirty);
    // void set_sign(int x, int y, int z, int face, const char *text);
    // void toggle_light(int x, int y, int z);
    // void set_light(int p, int q, int x, int y, int z, int w);
    // void _set_block(int p, int q, int x, int y, int z, int w, int dirty);
    // void set_block(int x, int y, int z, int w);
    // void record_block(int x, int y, int z, int w);
    // int get_block(int x, int y, int z);
    // void builder_block(int x, int y, int z, int w);
    // int render_chunks(Attrib *attrib, Player *player);
    // void render_signs(Attrib *attrib, Player *player);
    // void render_sign(Attrib *attrib, Player *player);
    // void render_players(Attrib *attrib, Player *player);
    // void render_sky(Attrib *attrib, Player *player, GLuint buffer);
    // void render_wireframe(Attrib *attrib, Player *player);
    // void render_crosshairs(Attrib *attrib);
    // void render_item(Attrib *attrib);
    // void render_text(Attrib *attrib, int justify, float x, float y, float n, char *text);
    // void add_message(const char *text);
    // void copy();
    // void paste();
    // void array(Block *b1, Block *b2, int xc, int yc, int zc);
    // void cube(Block *b1, Block *b2, int fill);
    // void sphere(Block *center, int radius, int fill, int fx, int fy, int fz);
    // void cylinder(Block *b1, Block *b2, int radius, int fill);
    // void tree(Block *block);
    // void parse_command(const char *buffer, int forward);
    // void on_light();
    // void on_left_click();
    // void on_right_click();
    // void on_middle_click();
    // int handle_events(double dt);
    // void create_window_and_context();
    // void parse_buffer(char *buffer);
    // void reset_model();

    void init();
    bool cleanup() const;
};

#endif // CRAFT_H
