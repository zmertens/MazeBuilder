#ifndef _db_h_
#define _db_h_

#include <vector>
#include <tuple>

#include "map.h"
#include "sign.h"

void db_enable();
void db_disable();
int get_db_enabled();
int db_init(char *path);
void db_close();
void db_commit();
void db_save_state(float x, float y, float z, float rx, float ry);
int db_load_state(float *x, float *y, float *z, float *rx, float *ry);
void db_insert_block(int p, int q, int x, int y, int z, int w);
void db_insert_blocks(const std::vector<std::tuple<int, int, int, int, int, int>>& blocks);
void db_insert_light(int p, int q, int x, int y, int z, int w);
void db_insert_sign(
    int p, int q, int x, int y, int z, int face, const char *text);
void db_delete_sign(int x, int y, int z, int face);
void db_delete_signs(int x, int y, int z);
void db_delete_all_signs();
void db_load_blocks(Map *map, int p, int q);
void db_load_lights(Map *map, int p, int q);
void db_load_signs(SignList *list, int p, int q);
int db_get_key(int p, int q);
void db_set_key(int p, int q, int key);
void db_worker_start(const char *path);
void db_worker_stop();
int db_worker_run(void *arg);

#endif
