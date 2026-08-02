#ifndef DB_H
#define DB_H

#include <cstddef>
#include <string_view>
#include <tuple>
#include <vector>

class voxels_map;

constexpr std::size_t MAX_SIGN_LENGTH = 16;

struct sign
{
	int x;
	int y;
	int z;
	int face;
	char text[MAX_SIGN_LENGTH];
};

struct sign_list
{
	std::size_t capacity{};
	std::size_t size{};
	sign *data{};
};

void sign_list_alloc(sign_list *list, std::size_t capacity);
void sign_list_free(sign_list *list);
void sign_list_add(sign_list *list, int x, int y, int z, int face, std::string_view text);
int sign_list_remove(sign_list *list, int x, int y, int z, int face);
int sign_list_remove_all(sign_list *list, int x, int y, int z);

void db_enable();
void db_disable();
bool db_is_enabled();
int db_init(const char* path);
void db_close();
void db_commit();
void db_flush();
void db_save_state(float x, float y, float z, float rx, float ry);
int db_load_state(float* x, float* y, float* z, float* rx, float* ry);
void db_insert_block(int p, int q, int x, int y, int z, int w);
void db_insert_blocks(const std::vector<std::tuple<int, int, int, int, int, int>>& blocks);
void db_insert_light(int p, int q, int x, int y, int z, int w);
void db_insert_sign(int p, int q, int x, int y, int z, int face, const char* text);
void db_delete_sign(int x, int y, int z, int face);
void db_delete_signs(int x, int y, int z);
void db_delete_all_signs();
void db_load_blocks(voxels_map* map, int p, int q);
void db_load_lights(voxels_map* map, int p, int q);
void db_load_signs(sign_list *list, int p, int q);
int db_get_key(int p, int q);
void db_set_key(int p, int q, int key);
std::vector<std::tuple<int, int, int, int>> db_query_blocks_near_chunks(int center_p, int center_q, int radius);
void db_worker_start(const char* path);
void db_worker_stop();
int db_worker_run(void* arg);

// Preview blocks functions - for temporary maze preview before committing
void db_insert_preview_blocks(int preview_id, const std::vector<std::tuple<int, int, int, int, int, int>>& blocks);
void db_load_preview_blocks(voxels_map* map, int p, int q, int preview_id);
int db_get_latest_preview_id();
void db_commit_latest_preview_to_main();
void db_flush_all_preview_blocks();
std::vector<std::tuple<int, int, int, int, int, int>> db_get_all_preview_blocks(int preview_id);


#endif // DB_H
