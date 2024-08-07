#include <string.h>
#include "db.h"
#include "ring.h"
#include <sqlite/sqlite3.h>

#include <thread>
#include <mutex>
#include <condition_variable>

static int db_enabled = 0;

static sqlite3 *db;
static sqlite3_stmt *insert_block_stmt;
static sqlite3_stmt *insert_light_stmt;
static sqlite3_stmt *insert_sign_stmt;
static sqlite3_stmt *delete_sign_stmt;
static sqlite3_stmt *delete_signs_stmt;
static sqlite3_stmt *load_blocks_stmt;
static sqlite3_stmt *load_lights_stmt;
static sqlite3_stmt *load_signs_stmt;
static sqlite3_stmt *get_key_stmt;
static sqlite3_stmt *set_key_stmt;

static Ring ring;

using namespace std;

static thread thrd;
static mutex mtx;
static condition_variable cnd;
static mutex load_mtx;

void db_enable() {
    db_enabled = 1;
}

void db_disable() {
    db_enabled = 0;
}

int get_db_enabled() {
    return db_enabled;
}

int db_init(char *path) {
    if (!db_enabled) {
        return 0;
    }
    static const char *create_query =
        "create table if not exists state ("
        "   x float not null,"
        "   y float not null,"
        "   z float not null,"
        "   rx float not null,"
        "   ry float not null"
        ");"
        "create table if not exists block ("
        "    p int not null,"
        "    q int not null,"
        "    x int not null,"
        "    y int not null,"
        "    z int not null,"
        "    w int not null"
        ");"
        "create table if not exists light ("
        "    p int not null,"
        "    q int not null,"
        "    x int not null,"
        "    y int not null,"
        "    z int not null,"
        "    w int not null"
        ");"
        "create table if not exists key ("
        "    p int not null,"
        "    q int not null,"
        "    key int not null"
        ");"
        "create table if not exists sign ("
        "    p int not null,"
        "    q int not null,"
        "    x int not null,"
        "    y int not null,"
        "    z int not null,"
        "    face int not null,"
        "    text text not null"
        ");"
        "create unique index if not exists block_pqxyz_idx on block (p, q, x, y, z);"
        "create unique index if not exists light_pqxyz_idx on light (p, q, x, y, z);"
        "create unique index if not exists key_pq_idx on key (p, q);"
        "create unique index if not exists sign_xyzface_idx on sign (x, y, z, face);"
        "create index if not exists sign_pq_idx on sign (p, q);";
    static const char *insert_block_query =
        "insert or replace into block (p, q, x, y, z, w) "
        "values (?, ?, ?, ?, ?, ?);";
    static const char *insert_light_query =
        "insert or replace into light (p, q, x, y, z, w) "
        "values (?, ?, ?, ?, ?, ?);";
    static const char *insert_sign_query =
        "insert or replace into sign (p, q, x, y, z, face, text) "
        "values (?, ?, ?, ?, ?, ?, ?);";
    static const char *delete_sign_query =
        "delete from sign where x = ? and y = ? and z = ? and face = ?;";
    static const char *delete_signs_query =
        "delete from sign where x = ? and y = ? and z = ?;";
    static const char *load_blocks_query =
        "select x, y, z, w from block where p = ? and q = ?;";
    static const char *load_lights_query =
        "select x, y, z, w from light where p = ? and q = ?;";
    static const char *load_signs_query =
        "select x, y, z, face, text from sign where p = ? and q = ?;";
    static const char *get_key_query =
        "select key from key where p = ? and q = ?;";
    static const char *set_key_query =
        "insert or replace into key (p, q, key) "
        "values (?, ?, ?);";
    int rc;
#if defined(__EMSCRIPTEN__)
    rc = sqlite3_open(":memory:", &db);
#else
    rc = sqlite3_open(path, &db);
#endif
    if (rc) return rc;
    rc = sqlite3_exec(db, create_query, nullptr, nullptr, nullptr);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(
        db, insert_block_query, -1, &insert_block_stmt, nullptr);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(
        db, insert_light_query, -1, &insert_light_stmt, nullptr);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(
        db, insert_sign_query, -1, &insert_sign_stmt, nullptr);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(
        db, delete_sign_query, -1, &delete_sign_stmt, nullptr);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(
        db, delete_signs_query, -1, &delete_signs_stmt, nullptr);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(db, load_blocks_query, -1, &load_blocks_stmt, nullptr);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(db, load_lights_query, -1, &load_lights_stmt, nullptr);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(db, load_signs_query, -1, &load_signs_stmt, nullptr);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(db, get_key_query, -1, &get_key_stmt, nullptr);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(db, set_key_query, -1, &set_key_stmt, nullptr);
    if (rc) return rc;
    sqlite3_exec(db, "begin;", nullptr, nullptr, nullptr);
    static constexpr auto p = "";
    db_worker_start(p);
    return 0;
}

void db_close() {
    if (!db_enabled) {
        return;
    }
    db_worker_stop();
    sqlite3_exec(db, "commit;", nullptr, nullptr, nullptr);
    sqlite3_finalize(insert_block_stmt);
    sqlite3_finalize(insert_light_stmt);
    sqlite3_finalize(insert_sign_stmt);
    sqlite3_finalize(delete_sign_stmt);
    sqlite3_finalize(delete_signs_stmt);
    sqlite3_finalize(load_blocks_stmt);
    sqlite3_finalize(load_lights_stmt);
    sqlite3_finalize(load_signs_stmt);
    sqlite3_finalize(get_key_stmt);
    sqlite3_finalize(set_key_stmt);
    sqlite3_close(db);
}

void db_commit() {
    if (!db_enabled) {
        return;
    }

    mtx.lock();
    ring_put_commit(&ring);
    cnd.notify_one();
    mtx.unlock();
}

void _db_commit() {
    sqlite3_exec(db, "commit; begin;", nullptr, nullptr, nullptr);
}


void db_save_state(float x, float y, float z, float rx, float ry) {
    if (!db_enabled) {
        return;
    }
    static const char *query =
        "insert into state (x, y, z, rx, ry) values (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    sqlite3_exec(db, "delete from state;", nullptr, nullptr, nullptr);
    sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, x);
    sqlite3_bind_double(stmt, 2, y);
    sqlite3_bind_double(stmt, 3, z);
    sqlite3_bind_double(stmt, 4, rx);
    sqlite3_bind_double(stmt, 5, ry);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

int db_load_state(float *x, float *y, float *z, float *rx, float *ry) {
    if (!db_enabled) {
        return 0;
    }
    static const char *query =
        "select x, y, z, rx, ry from state;";
    int result = 0;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *x = sqlite3_column_double(stmt, 0);
        *y = sqlite3_column_double(stmt, 1);
        *z = sqlite3_column_double(stmt, 2);
        *rx = sqlite3_column_double(stmt, 3);
        *ry = sqlite3_column_double(stmt, 4);
        result = 1;
    }
    sqlite3_finalize(stmt);
    return result;
}

void db_insert_block(int p, int q, int x, int y, int z, int w) {
    if (!db_enabled) {
        return;
    }
    mtx.lock();
    ring_put_block(&ring, p, q, x, y, z, w);
    cnd.notify_one();
    mtx.unlock();
}

void _db_insert_block(int p, int q, int x, int y, int z, int w) {
    sqlite3_reset(insert_block_stmt);
    sqlite3_bind_int(insert_block_stmt, 1, p);
    sqlite3_bind_int(insert_block_stmt, 2, q);
    sqlite3_bind_int(insert_block_stmt, 3, x);
    sqlite3_bind_int(insert_block_stmt, 4, y);
    sqlite3_bind_int(insert_block_stmt, 5, z);
    sqlite3_bind_int(insert_block_stmt, 6, w);
    sqlite3_step(insert_block_stmt);
}

void db_insert_blocks(const std::vector<std::tuple<int, int, int, int, int, int>>& blocks) {
    if (!db_enabled) {
        return;
    }
    mtx.lock();
    int* blocks_array = new int[blocks.size() * 6];
    memcpy(blocks_array, blocks.data(), blocks.size() * 6 * sizeof(int));
    ring_put_blocks(&ring, blocks_array);
    cnd.notify_one();
    mtx.unlock();
}

void _db_insert_blocks(int* blocks) {
    for (int i = 0; i < sizeof(*blocks) / 6; i++) {
        sqlite3_reset(insert_block_stmt);
        sqlite3_bind_int(insert_block_stmt, 1, blocks[i * 6]);
        sqlite3_bind_int(insert_block_stmt, 2, blocks[i * 6 + 1]);
        sqlite3_bind_int(insert_block_stmt, 3, blocks[i * 6 + 2]);
        sqlite3_bind_int(insert_block_stmt, 4, blocks[i * 6 + 3]);
        sqlite3_bind_int(insert_block_stmt, 5, blocks[i * 6 + 4]);
        sqlite3_bind_int(insert_block_stmt, 6, blocks[i * 6 + 5]);
        sqlite3_step(insert_block_stmt);
    }
}

void db_insert_light(int p, int q, int x, int y, int z, int w) {
    if (!db_enabled) {
        return;
    }

    mtx.lock();
    ring_put_light(&ring, p, q, x, y, z, w);
    cnd.notify_one();
    mtx.unlock();
}

void _db_insert_light(int p, int q, int x, int y, int z, int w) {
    sqlite3_reset(insert_light_stmt);
    sqlite3_bind_int(insert_light_stmt, 1, p);
    sqlite3_bind_int(insert_light_stmt, 2, q);
    sqlite3_bind_int(insert_light_stmt, 3, x);
    sqlite3_bind_int(insert_light_stmt, 4, y);
    sqlite3_bind_int(insert_light_stmt, 5, z);
    sqlite3_bind_int(insert_light_stmt, 6, w);
    sqlite3_step(insert_light_stmt);
}

void db_insert_sign(
    int p, int q, int x, int y, int z, int face, const char *text)
{
    if (!db_enabled) {
        return;
    }
    sqlite3_reset(insert_sign_stmt);
    sqlite3_bind_int(insert_sign_stmt, 1, p);
    sqlite3_bind_int(insert_sign_stmt, 2, q);
    sqlite3_bind_int(insert_sign_stmt, 3, x);
    sqlite3_bind_int(insert_sign_stmt, 4, y);
    sqlite3_bind_int(insert_sign_stmt, 5, z);
    sqlite3_bind_int(insert_sign_stmt, 6, face);
    sqlite3_bind_text(insert_sign_stmt, 7, text, -1, nullptr);
    sqlite3_step(insert_sign_stmt);
}

void db_delete_sign(int x, int y, int z, int face) {
    if (!db_enabled) {
        return;
    }
    sqlite3_reset(delete_sign_stmt);
    sqlite3_bind_int(delete_sign_stmt, 1, x);
    sqlite3_bind_int(delete_sign_stmt, 2, y);
    sqlite3_bind_int(delete_sign_stmt, 3, z);
    sqlite3_bind_int(delete_sign_stmt, 4, face);
    sqlite3_step(delete_sign_stmt);
}

void db_delete_signs(int x, int y, int z) {
    if (!db_enabled) {
        return;
    }
    sqlite3_reset(delete_signs_stmt);
    sqlite3_bind_int(delete_signs_stmt, 1, x);
    sqlite3_bind_int(delete_signs_stmt, 2, y);
    sqlite3_bind_int(delete_signs_stmt, 3, z);
    sqlite3_step(delete_signs_stmt);
}

void db_delete_all_signs() {
    if (!db_enabled) {
        return;
    }
    sqlite3_exec(db, "delete from sign;", nullptr, nullptr, nullptr);
}

void db_load_blocks(Map *map, int p, int q) {
    if (!db_enabled) {
        return;
    }
    load_mtx.lock();
    sqlite3_reset(load_blocks_stmt);
    sqlite3_bind_int(load_blocks_stmt, 1, p);
    sqlite3_bind_int(load_blocks_stmt, 2, q);
    while (sqlite3_step(load_blocks_stmt) == SQLITE_ROW) {
        int x = sqlite3_column_int(load_blocks_stmt, 0);
        int y = sqlite3_column_int(load_blocks_stmt, 1);
        int z = sqlite3_column_int(load_blocks_stmt, 2);
        int w = sqlite3_column_int(load_blocks_stmt, 3);
        map_set(map, x, y, z, w);
    }
    load_mtx.unlock();
}

void db_load_lights(Map *map, int p, int q) {
    if (!db_enabled) {
        return;
    }
    load_mtx.lock();
    sqlite3_reset(load_lights_stmt);
    sqlite3_bind_int(load_lights_stmt, 1, p);
    sqlite3_bind_int(load_lights_stmt, 2, q);
    while (sqlite3_step(load_lights_stmt) == SQLITE_ROW) {
        int x = sqlite3_column_int(load_lights_stmt, 0);
        int y = sqlite3_column_int(load_lights_stmt, 1);
        int z = sqlite3_column_int(load_lights_stmt, 2);
        int w = sqlite3_column_int(load_lights_stmt, 3);
        map_set(map, x, y, z, w);
    }
    load_mtx.unlock();
}

void db_load_signs(SignList *list, int p, int q) {
    if (!db_enabled) {
        return;
    }
    sqlite3_reset(load_signs_stmt);
    sqlite3_bind_int(load_signs_stmt, 1, p);
    sqlite3_bind_int(load_signs_stmt, 2, q);
    while (sqlite3_step(load_signs_stmt) == SQLITE_ROW) {
        int x = sqlite3_column_int(load_signs_stmt, 0);
        int y = sqlite3_column_int(load_signs_stmt, 1);
        int z = sqlite3_column_int(load_signs_stmt, 2);
        int face = sqlite3_column_int(load_signs_stmt, 3);
        const char *text = (const char *)sqlite3_column_text(
            load_signs_stmt, 4);
        sign_list_add(list, x, y, z, face, text);
    }
}

int db_get_key(int p, int q) {
    if (!db_enabled) {
        return 0;
    }
    sqlite3_reset(get_key_stmt);
    sqlite3_bind_int(get_key_stmt, 1, p);
    sqlite3_bind_int(get_key_stmt, 2, q);
    if (sqlite3_step(get_key_stmt) == SQLITE_ROW) {
        return sqlite3_column_int(get_key_stmt, 0);
    }
    return 0;
}

void db_set_key(int p, int q, int key) {
    if (!db_enabled) {
        return;
    }
    mtx.lock();
    ring_put_key(&ring, p, q, key);
    cnd.notify_one();
    mtx.unlock();
}

void _db_set_key(int p, int q, int key) {
    sqlite3_reset(set_key_stmt);
    sqlite3_bind_int(set_key_stmt, 1, p);
    sqlite3_bind_int(set_key_stmt, 2, q);
    sqlite3_bind_int(set_key_stmt, 3, key);
    sqlite3_step(set_key_stmt);
}

void db_worker_start(const char *path) {
    if (!db_enabled) {
        return;
    }
    ring_alloc(&ring, 1024);

    thrd = thread([](const char* p) {
        db_worker_run(reinterpret_cast<void*>(const_cast<char*>(p)));
    }, path);
}

void db_worker_stop() {
    if (!db_enabled) {
        return;
    }

    mtx.lock();
    ring_put_exit(&ring);

    cnd.notify_one();

    mtx.unlock();

    thrd.join();

    ring_free(&ring);
}

int db_worker_run(void *arg) {
    int running = 1;
    while (running) {
        RingEntry e;

        while (!ring_get(&ring, &e)) {
            unique_lock<mutex> u_lck (mtx);
            cnd.wait(u_lck);
        }

        switch (e.type) {
            case BLOCK:
                _db_insert_block(e.p, e.q, e.x, e.y, e.z, e.w);
                break;
            case BLOCKS:
				_db_insert_blocks(e.blocks);
				break;
            case LIGHT:
                _db_insert_light(e.p, e.q, e.x, e.y, e.z, e.w);
                break;
            case KEY:
                _db_set_key(e.p, e.q, e.key);
                break;
            case COMMIT:
                _db_commit();
                break;
            case EXIT:
                running = 0;
                break;
        }
    }
    return 0;
}
