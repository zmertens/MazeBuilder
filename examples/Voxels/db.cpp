#include "db.h"

#include "voxels_map.h"

#include <sqlite/sqlite3.h>

#include <SDL3/SDL.h>

#include <chrono>

#include <condition_variable>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

namespace
{
    enum class ring_entry_type
    {
        BLOCK,
        // Part of a vector
        BLOCKS,
        LIGHT,
        KEY,
        COMMIT,
        EXIT
    };

    struct ring_entry
    {
        ring_entry_type type;
        int p;
        int q;
        int x;
        int y;
        int z;
        int w;
        int *blocks;
        int key;
    };

    struct ring
    {
        unsigned int capacity;
        unsigned int start;
        unsigned int end;
        ring_entry *data;
    };

    void ring_alloc(ring *r, const int capacity)
    {
        r->capacity = capacity;
        r->start = 0;
        r->end = 0;
        r->data = static_cast<ring_entry *>(SDL_calloc(capacity, sizeof(ring_entry)));
    }

    void ring_free(const ring *r)
    {
        SDL_free(r->data);
    }

    int ring_empty(const ring *r)
    {
        return r->start == r->end;
    }

    int ring_full(const ring *r)
    {
        return r->start == (r->end + 1) % r->capacity;
    }

    int ring_size(const ring *r)
    {
        if (r->end >= r->start)
        {
            return r->end - r->start;
        }
        return r->capacity - (r->start - r->end);
    }

    int ring_get(ring *r, ring_entry *entry)
    {
        if (ring_empty(r))
        {
            return 0;
        }

        const ring_entry *e = r->data + r->start;
        SDL_memcpy(entry, e, sizeof(ring_entry));
        r->start = (r->start + 1) % r->capacity;
        return 1;
    }

    void ring_put(ring *r, const ring_entry *entry)
    {
        auto ring_grow = [](ring *r)
        {
            ring new_ring;
            ring_entry entry;
            ring_alloc(&new_ring, r->capacity * 2);
            while (ring_get(r, &entry))
            {
                ring_put(&new_ring, &entry);
            }
            SDL_free(r->data);
            r->capacity = new_ring.capacity;
            r->start = new_ring.start;
            r->end = new_ring.end;
            r->data = new_ring.data;
        };

        if (ring_full(r))
        {
            ring_grow(r);
        }
        ring_entry *e = r->data + r->end;
        SDL_memcpy(e, entry, sizeof(ring_entry));
        r->end = (r->end + 1) % r->capacity;
    }

    void ring_put_block(ring *r, const int p, const int q, const int x, const int y, const int z, const int w)
    {
        ring_entry entry;
        entry.type = ring_entry_type::BLOCK;
        entry.p = p;
        entry.q = q;
        entry.x = x;
        entry.y = y;
        entry.z = z;
        entry.w = w;
        entry.blocks = nullptr;
        ring_put(r, &entry);
    }

    void ring_put_blocks(ring *r, int *blocks)
    {
        ring_entry entry;
        entry.type = ring_entry_type::BLOCKS;
        entry.p = 0;
        entry.q = 0;
        entry.x = 0;
        entry.y = 0;
        entry.z = 0;
        entry.w = 0;
        entry.blocks = blocks;
        ring_put(r, &entry);
    }

    void ring_put_light(ring *r, const int p, const int q, const int x, const int y, const int z, const int w)
    {
        ring_entry entry;
        entry.type = ring_entry_type::LIGHT;
        entry.p = p;
        entry.q = q;
        entry.x = x;
        entry.y = y;
        entry.z = z;
        entry.w = w;
        ring_put(r, &entry);
    }

    void ring_put_key(ring *r, const int p, const int q, const int key)
    {
        ring_entry entry;
        entry.type = ring_entry_type::KEY;
        entry.p = p;
        entry.q = q;
        entry.key = key;
        ring_put(r, &entry);
    }

    void ring_put_commit(ring *r)
    {
        ring_entry entry;
        entry.type = ring_entry_type::COMMIT;
        ring_put(r, &entry);
    }

    void ring_put_exit(ring *r)
    {
        ring_entry entry;
        entry.type = ring_entry_type::EXIT;
        ring_put(r, &entry);
    }

    void sign_list_grow(sign_list *list)
    {
        sign_list new_list;
        new_list.capacity = list->capacity * 2;
        new_list.size = list->size;
        new_list.data = static_cast<sign *>(SDL_calloc(new_list.capacity, sizeof(sign)));
        SDL_memcpy(new_list.data, list->data, list->size * sizeof(sign));
        SDL_free(list->data);
        list->capacity = new_list.capacity;
        list->size = new_list.size;
        list->data = new_list.data;
    }

    void _sign_list_add(sign_list *list, const sign *entry)
    {
        if (list->size == list->capacity)
        {
            sign_list_grow(list);
        }
        sign *e = list->data + list->size++;
        SDL_memcpy(e, entry, sizeof(sign));
    }

    int sign_list_remove_impl(sign_list *list, const int x, const int y, const int z, const int face)
    {
        int result = 0;
        for (std::size_t i = 0; i < list->size; i++)
        {
            if (sign *e = list->data + i; e->x == x && e->y == y && e->z == z && e->face == face)
            {
                const sign *other = list->data + (--list->size);
                SDL_memcpy(e, other, sizeof(sign));
                i--;
                result++;
            }
        }
        return result;
    }

    int sign_list_remove_all_impl(sign_list *list, const int x, const int y, const int z)
    {
        int result = 0;
        for (std::size_t i = 0; i < list->size; i++)
        {
            if (sign *e = list->data + i; e->x == x && e->y == y && e->z == z)
            {
                const sign *other = list->data + --list->size;
                SDL_memcpy(e, other, sizeof(sign));
                i--;
                result++;
            }
        }
        return result;
    }

} // namespace

void sign_list_alloc(sign_list *list, const std::size_t capacity)
{
    list->capacity = capacity;
    list->size = 0;
    list->data = static_cast<sign *>(SDL_calloc(capacity, sizeof(sign)));
}

void sign_list_free(sign_list *list)
{
    SDL_free(list->data);
    list->data = nullptr;
    list->size = 0;
    list->capacity = 0;
}

void sign_list_add(sign_list *list, const int x, const int y, const int z, const int face, const std::string_view text)
{
    sign_list_remove_impl(list, x, y, z, face);
    sign entry;
    entry.x = x;
    entry.y = y;
    entry.z = z;
    entry.face = face;
    SDL_strlcpy(entry.text, text.data(), MAX_SIGN_LENGTH);
    entry.text[MAX_SIGN_LENGTH - 1] = '\0';
    _sign_list_add(list, &entry);
}

int sign_list_remove(sign_list *list, const int x, const int y, const int z, const int face)
{
    return sign_list_remove_impl(list, x, y, z, face);
}

int sign_list_remove_all(sign_list *list, const int x, const int y, const int z)
{
    return sign_list_remove_all_impl(list, x, y, z);
}

static bool db_enabled = false;

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

// Preview blocks database - for temporary maze previews
static sqlite3_stmt *insert_preview_block_stmt;
static sqlite3_stmt *load_preview_blocks_stmt;
static sqlite3_stmt *delete_all_preview_blocks_stmt;
static sqlite3_stmt *move_latest_preview_to_main_stmt;
static sqlite3_stmt *get_latest_preview_id_stmt;

static ring s_ring;

static std::thread db_thread;
static std::mutex mtx;
static std::condition_variable cnd;
static std::mutex load_mtx;

void db_enable()
{
    db_enabled = true;
}

void db_disable()
{
    db_enabled = false;
}

bool db_is_enabled()
{
    return db_enabled;
}

int db_init(const char *path)
{
    if (!db_enabled)
    {
        return 0;
    }
    static auto create_query =
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
        "create table if not exists preview_blocks ("
        "    preview_id int not null,"
        "    p int not null,"
        "    q int not null,"
        "    x int not null,"
        "    y int not null,"
        "    z int not null,"
        "    w int not null,"
        "    timestamp datetime default current_timestamp"
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
        "create index if not exists block_pqw_idx on block (p, q, w);" // Optimized for range queries
        "create index if not exists preview_blocks_id_idx on preview_blocks (preview_id);"
        "create index if not exists preview_blocks_pqxyz_idx on preview_blocks (p, q, x, y, z);"
        "create unique index if not exists light_pqxyz_idx on light (p, q, x, y, z);"
        "create unique index if not exists key_pq_idx on key (p, q);"
        "create unique index if not exists sign_xyzface_idx on sign (x, y, z, face);"
        "create index if not exists sign_pq_idx on sign (p, q);";
    static auto insert_block_query =
        "insert or replace into block (p, q, x, y, z, w) "
        "values (?, ?, ?, ?, ?, ?);";
    static auto insert_light_query =
        "insert or replace into light (p, q, x, y, z, w) "
        "values (?, ?, ?, ?, ?, ?);";
    static auto insert_sign_query =
        "insert or replace into sign (p, q, x, y, z, face, text) "
        "values (?, ?, ?, ?, ?, ?, ?);";
    static auto delete_sign_query =
        "delete from sign where x = ? and y = ? and z = ? and face = ?;";
    static auto delete_signs_query =
        "delete from sign where x = ? and y = ? and z = ?;";
    static auto load_blocks_query =
        "select x, y, z, w from block where p = ? and q = ?;";
    static auto load_lights_query =
        "select x, y, z, w from light where p = ? and q = ?;";
    static auto load_signs_query =
        "select x, y, z, face, text from sign where p = ? and q = ?;";
    static auto get_key_query =
        "select key from key where p = ? and q = ?;";
    static auto set_key_query =
        "insert or replace into key (p, q, key) "
        "values (?, ?, ?);";

    // Preview blocks queries
    static auto insert_preview_block_query =
        "insert into preview_blocks (preview_id, p, q, x, y, z, w) "
        "values (?, ?, ?, ?, ?, ?, ?);";
    static auto load_preview_blocks_query =
        "select x, y, z, w from preview_blocks where p = ? and q = ? and preview_id = ?;";
    static auto delete_all_preview_blocks_query =
        "delete from preview_blocks;";
    static auto get_latest_preview_id_query =
        "select max(preview_id) from preview_blocks;";

    auto check_rc = [](int rc)
    {
        if (rc != SQLITE_OK)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "SQLite error: %s\n", sqlite3_errmsg(db));
        }
    };

    auto rc = sqlite3_open(path, &db);
    check_rc(rc);
    rc = sqlite3_exec(db, create_query, nullptr, nullptr, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(
        db, insert_block_query, -1, &insert_block_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(
        db, insert_light_query, -1, &insert_light_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(
        db, insert_sign_query, -1, &insert_sign_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(
        db, delete_sign_query, -1, &delete_sign_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(
        db, delete_signs_query, -1, &delete_signs_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(db, load_blocks_query, -1, &load_blocks_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(db, load_lights_query, -1, &load_lights_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(db, load_signs_query, -1, &load_signs_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(db, get_key_query, -1, &get_key_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(db, set_key_query, -1, &set_key_stmt, nullptr);
    check_rc(rc);

    // Prepare preview blocks statements
    rc = sqlite3_prepare_v2(db, insert_preview_block_query, -1, &insert_preview_block_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(db, load_preview_blocks_query, -1, &load_preview_blocks_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(db, delete_all_preview_blocks_query, -1, &delete_all_preview_blocks_stmt, nullptr);
    check_rc(rc);
    rc = sqlite3_prepare_v2(db, get_latest_preview_id_query, -1, &get_latest_preview_id_stmt, nullptr);
    check_rc(rc);
    sqlite3_exec(db, "begin;", nullptr, nullptr, nullptr);
    static constexpr auto p = "";
    db_worker_start(p);

    return 0;
}

void db_close()
{
    if (!db_enabled)
    {
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

    // Finalize preview blocks statements
    sqlite3_finalize(insert_preview_block_stmt);
    sqlite3_finalize(load_preview_blocks_stmt);
    sqlite3_finalize(delete_all_preview_blocks_stmt);
    sqlite3_finalize(get_latest_preview_id_stmt);

    sqlite3_close(db);
}

void db_commit()
{
    if (!db_enabled)
    {
        return;
    }

    mtx.lock();
    ring_put_commit(&s_ring);
    cnd.notify_one();
    mtx.unlock();
}

void _db_commit()
{
    sqlite3_exec(db, "commit; begin;", nullptr, nullptr, nullptr);
}

void db_flush()
{
    if (!db_enabled)
    {
        return;
    }

    // Execute DELETE statements to clear all tables
    const char *flush_query =
        "delete from state;"
        "delete from block;"
        "delete from light;"
        "delete from key;"
        "delete from sign;";

    int rc = sqlite3_exec(db, flush_query, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to flush database: %s\n", sqlite3_errmsg(db));
    }
    else
    {
        SDL_Log("Database flushed successfully\n");
        // Commit the deletions
        _db_commit();
    }
}

void db_save_state(float x, float y, float z, float rx, float ry)
{
    if (!db_enabled)
    {
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

int db_load_state(float *x, float *y, float *z, float *rx, float *ry)
{
    if (!db_enabled)
    {
        return 0;
    }
    static const char *query =
        "select x, y, z, rx, ry from state;";
    int result = 0;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
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

void db_insert_block(int p, int q, int x, int y, int z, int w)
{
    if (!db_enabled)
    {
        return;
    }
    mtx.lock();
    ring_put_block(&s_ring, p, q, x, y, z, w);
    cnd.notify_one();
    mtx.unlock();
}

void _db_insert_block(int p, int q, int x, int y, int z, int w)
{
    sqlite3_reset(insert_block_stmt);
    sqlite3_bind_int(insert_block_stmt, 1, p);
    sqlite3_bind_int(insert_block_stmt, 2, q);
    sqlite3_bind_int(insert_block_stmt, 3, x);
    sqlite3_bind_int(insert_block_stmt, 4, y);
    sqlite3_bind_int(insert_block_stmt, 5, z);
    sqlite3_bind_int(insert_block_stmt, 6, w);
    sqlite3_step(insert_block_stmt);
}

void db_insert_blocks(const std::vector<std::tuple<int, int, int, int, int, int>> &blocks)
{
    if (!db_enabled)
    {
        return;
    }
    mtx.lock();
    int *blocks_array = new int[blocks.size() * 6];
    SDL_memcpy(blocks_array, blocks.data(), blocks.size() * 6 * sizeof(int));
    ring_put_blocks(&s_ring, blocks_array);
    cnd.notify_one();
    mtx.unlock();
}

void _db_insert_blocks(int *blocks)
{
    for (int i = 0; i < sizeof(*blocks) / 6; i++)
    {
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

void db_insert_light(int p, int q, int x, int y, int z, int w)
{
    if (!db_enabled)
    {
        return;
    }

    mtx.lock();
    ring_put_light(&s_ring, p, q, x, y, z, w);
    cnd.notify_one();
    mtx.unlock();
}

void _db_insert_light(int p, int q, int x, int y, int z, int w)
{
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
    if (!db_enabled)
    {
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

void db_delete_sign(int x, int y, int z, int face)
{
    if (!db_enabled)
    {
        return;
    }
    sqlite3_reset(delete_sign_stmt);
    sqlite3_bind_int(delete_sign_stmt, 1, x);
    sqlite3_bind_int(delete_sign_stmt, 2, y);
    sqlite3_bind_int(delete_sign_stmt, 3, z);
    sqlite3_bind_int(delete_sign_stmt, 4, face);
    sqlite3_step(delete_sign_stmt);
}

void db_delete_signs(int x, int y, int z)
{
    if (!db_enabled)
    {
        return;
    }
    sqlite3_reset(delete_signs_stmt);
    sqlite3_bind_int(delete_signs_stmt, 1, x);
    sqlite3_bind_int(delete_signs_stmt, 2, y);
    sqlite3_bind_int(delete_signs_stmt, 3, z);
    sqlite3_step(delete_signs_stmt);
}

void db_delete_all_signs()
{
    if (!db_enabled)
    {
        return;
    }
    sqlite3_exec(db, "delete from sign;", nullptr, nullptr, nullptr);
}

void db_load_blocks(voxels_map *map, int p, int q)
{
    if (!db_enabled)
    {
        return;
    }
    load_mtx.lock();
    sqlite3_reset(load_blocks_stmt);
    sqlite3_bind_int(load_blocks_stmt, 1, p);
    sqlite3_bind_int(load_blocks_stmt, 2, q);
    while (sqlite3_step(load_blocks_stmt) == SQLITE_ROW)
    {
        int x = sqlite3_column_int(load_blocks_stmt, 0);
        int y = sqlite3_column_int(load_blocks_stmt, 1);
        int z = sqlite3_column_int(load_blocks_stmt, 2);
        int w = sqlite3_column_int(load_blocks_stmt, 3);
        map->set(x, y, z, w);
    }
    load_mtx.unlock();
}

void db_load_lights(voxels_map *map, int p, int q)
{
    if (!db_enabled)
    {
        return;
    }
    load_mtx.lock();
    sqlite3_reset(load_lights_stmt);
    sqlite3_bind_int(load_lights_stmt, 1, p);
    sqlite3_bind_int(load_lights_stmt, 2, q);
    while (sqlite3_step(load_lights_stmt) == SQLITE_ROW)
    {
        int x = sqlite3_column_int(load_lights_stmt, 0);
        int y = sqlite3_column_int(load_lights_stmt, 1);
        int z = sqlite3_column_int(load_lights_stmt, 2);
        int w = sqlite3_column_int(load_lights_stmt, 3);
        map->set(x, y, z, w);
    }
    load_mtx.unlock();
}

void db_load_signs(sign_list *list, int p, int q)
{
    if (!db_enabled)
    {
        return;
    }
    sqlite3_reset(load_signs_stmt);
    sqlite3_bind_int(load_signs_stmt, 1, p);
    sqlite3_bind_int(load_signs_stmt, 2, q);
    while (sqlite3_step(load_signs_stmt) == SQLITE_ROW)
    {
        const int x = sqlite3_column_int(load_signs_stmt, 0);
        const int y = sqlite3_column_int(load_signs_stmt, 1);
        const int z = sqlite3_column_int(load_signs_stmt, 2);
        const int face = sqlite3_column_int(load_signs_stmt, 3);
        const auto text = reinterpret_cast<const char *>(sqlite3_column_text(load_signs_stmt, 4));
        sign_list_add(list, x, y, z, face, text);
    }
}

int db_get_key(const int p, const int q)
{
    if (!db_enabled)
    {
        return 0;
    }
    sqlite3_reset(get_key_stmt);
    sqlite3_bind_int(get_key_stmt, 1, p);
    sqlite3_bind_int(get_key_stmt, 2, q);
    if (sqlite3_step(get_key_stmt) == SQLITE_ROW)
    {
        return sqlite3_column_int(get_key_stmt, 0);
    }
    return 0;
}

void db_set_key(int p, int q, const int key)
{
    if (!db_enabled)
    {
        return;
    }
    mtx.lock();
    ring_put_key(&s_ring, p, q, key);
    cnd.notify_one();
    mtx.unlock();
}

void _db_set_key(const int p, const int q, const int key)
{
    sqlite3_reset(set_key_stmt);
    sqlite3_bind_int(set_key_stmt, 1, p);
    sqlite3_bind_int(set_key_stmt, 2, q);
    sqlite3_bind_int(set_key_stmt, 3, key);
    sqlite3_step(set_key_stmt);
}

std::vector<std::tuple<int, int, int, int>> db_query_blocks_near_chunks(int center_p, int center_q, int radius)
{
    std::vector<std::tuple<int, int, int, int>> blocks;

    if (!db_enabled)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Database not enabled, cannot query blocks\n");
        return blocks;
    }

    const auto start_time = std::chrono::steady_clock::now();

    // Query blocks from chunks within radius of center chunk
    // The 'block' table has columns: p, q, x, y, z, w
    // p, q are chunk coordinates
    // x, y, z are world block coordinates
    // w is the block type/ID
    const char *query =
        "SELECT x, y, z, w FROM block "
        "WHERE p >= ? AND p <= ? AND q >= ? AND q <= ? "
        "AND w > 0 "; // Only non-air blocks - removed ORDER BY for performance

    sqlite3_stmt *stmt = nullptr;

    load_mtx.lock();
    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to prepare block query: %s\n",
                     sqlite3_errmsg(db));
        load_mtx.unlock();
        return blocks;
    }

    // Bind parameters
    sqlite3_bind_int(stmt, 1, center_p - radius);
    sqlite3_bind_int(stmt, 2, center_p + radius);
    sqlite3_bind_int(stmt, 3, center_q - radius);
    sqlite3_bind_int(stmt, 4, center_q + radius);
    load_mtx.unlock();

    // Execute query and collect results (no lock needed for reading)
    blocks.reserve(10000); // Reserve space to reduce reallocations
    int row_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int x = sqlite3_column_int(stmt, 0);
        int y = sqlite3_column_int(stmt, 1);
        int z = sqlite3_column_int(stmt, 2);
        int w = sqlite3_column_int(stmt, 3);

        blocks.emplace_back(x, y, z, w);
        row_count++;
    }

    sqlite3_finalize(stmt);

    return blocks;
}

void db_worker_start(const char *path)
{
    if (!db_enabled)
    {
        return;
    }
    ring_alloc(&s_ring, 1024);

    db_thread = std::thread([](const char *p)
                            { db_worker_run(const_cast<char *>(p)); }, path);
}

void db_worker_stop()
{
    if (!db_enabled)
    {
        return;
    }

    mtx.lock();
    ring_put_exit(&s_ring);

    cnd.notify_one();

    mtx.unlock();

    db_thread.join();

    ring_free(&s_ring);
}

int db_worker_run(void *arg)
{
    bool running = true;
    while (running)
    {
        ring_entry e;

        while (!ring_get(&s_ring, &e))
        {
            std::unique_lock my_lock(mtx);
            cnd.wait(my_lock);
        }

        switch (e.type)
        {
        case ring_entry_type::BLOCK:
            _db_insert_block(e.p, e.q, e.x, e.y, e.z, e.w);
            break;
        case ring_entry_type::BLOCKS:
            _db_insert_blocks(e.blocks);
            break;
        case ring_entry_type::LIGHT:
            _db_insert_light(e.p, e.q, e.x, e.y, e.z, e.w);
            break;
        case ring_entry_type::KEY:
            _db_set_key(e.p, e.q, e.key);
            break;
        case ring_entry_type::COMMIT:
            _db_commit();
            break;
        case ring_entry_type::EXIT:
            running = false;
            break;
        }
    }
    return 0;
}

// ============================================================================
// PREVIEW BLOCKS FUNCTIONS
// ============================================================================

// Insert multiple preview blocks at once
void db_insert_preview_blocks(int preview_id, const std::vector<std::tuple<int, int, int, int, int, int>> &blocks)
{
    if (!db_enabled || blocks.empty())
    {
        return;
    }

    load_mtx.lock();
    sqlite3_exec(db, "begin;", nullptr, nullptr, nullptr);

    for (const auto &[p, q, x, y, z, w] : blocks)
    {
        sqlite3_reset(insert_preview_block_stmt);
        sqlite3_bind_int(insert_preview_block_stmt, 1, preview_id);
        sqlite3_bind_int(insert_preview_block_stmt, 2, p);
        sqlite3_bind_int(insert_preview_block_stmt, 3, q);
        sqlite3_bind_int(insert_preview_block_stmt, 4, x);
        sqlite3_bind_int(insert_preview_block_stmt, 5, y);
        sqlite3_bind_int(insert_preview_block_stmt, 6, z);
        sqlite3_bind_int(insert_preview_block_stmt, 7, w);
        sqlite3_step(insert_preview_block_stmt);
    }

    sqlite3_exec(db, "commit;", nullptr, nullptr, nullptr);
    load_mtx.unlock();
}

// Load preview blocks for a specific chunk and preview_id
void db_load_preview_blocks(voxels_map *map, const int p, const int q, const int preview_id)
{
    if (!db_enabled)
    {
        return;
    }
    load_mtx.lock();
    sqlite3_reset(load_preview_blocks_stmt);
    sqlite3_bind_int(load_preview_blocks_stmt, 1, p);
    sqlite3_bind_int(load_preview_blocks_stmt, 2, q);
    sqlite3_bind_int(load_preview_blocks_stmt, 3, preview_id);
    while (sqlite3_step(load_preview_blocks_stmt) == SQLITE_ROW)
    {
        const int x = sqlite3_column_int(load_preview_blocks_stmt, 0);
        const int y = sqlite3_column_int(load_preview_blocks_stmt, 1);
        const int z = sqlite3_column_int(load_preview_blocks_stmt, 2);
        const int w = sqlite3_column_int(load_preview_blocks_stmt, 3);
        map->set(x, y, z, w);
    }
    load_mtx.unlock();
}

// Get the latest preview_id
int db_get_latest_preview_id()
{
    if (!db_enabled)
    {
        return 0;
    }

    load_mtx.lock();
    sqlite3_reset(get_latest_preview_id_stmt);
    int latest_id = 0;
    if (sqlite3_step(get_latest_preview_id_stmt) == SQLITE_ROW)
    {
        latest_id = sqlite3_column_int(get_latest_preview_id_stmt, 0);
    }
    load_mtx.unlock();

    return latest_id;
}

// Commit the latest preview to the main block table and clear all previews
void db_commit_latest_preview_to_main()
{
    if (!db_enabled)
    {
        return;
    }

    const int latest_preview_id = db_get_latest_preview_id();
    if (latest_preview_id <= 0)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No preview blocks to commit\n");
        return;
    }

    load_mtx.lock();

    // Move latest preview blocks to main block table
    const char *move_query =
        "insert or replace into block (p, q, x, y, z, w) "
        "select p, q, x, y, z, w from preview_blocks where preview_id = ?;";

    sqlite3_stmt *move_stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, move_query, -1, &move_stmt, nullptr);
    if (rc == SQLITE_OK)
    {
        sqlite3_bind_int(move_stmt, 1, latest_preview_id);
        sqlite3_step(move_stmt);
        sqlite3_finalize(move_stmt);

        const int moved_count = sqlite3_changes(db);
    }

    // Clear all preview blocks after committing
    sqlite3_reset(delete_all_preview_blocks_stmt);
    sqlite3_step(delete_all_preview_blocks_stmt);

    // Commit the transaction
    sqlite3_exec(db, "commit;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "begin;", nullptr, nullptr, nullptr);

    load_mtx.unlock();
}

// Flush all preview blocks without committing to main
void db_flush_all_preview_blocks()
{
    if (!db_enabled)
    {
        return;
    }

    load_mtx.lock();
    sqlite3_reset(delete_all_preview_blocks_stmt);
    sqlite3_step(delete_all_preview_blocks_stmt);
    load_mtx.unlock();

    SDL_Log("Flushed all preview blocks\n");
}

// Get all blocks for a specific preview_id (for building)
std::vector<std::tuple<int, int, int, int, int, int>> db_get_all_preview_blocks(int preview_id)
{
    std::vector<std::tuple<int, int, int, int, int, int>> blocks;

    if (!db_enabled)
    {
        return blocks;
    }

    load_mtx.lock();

    const char *query = "SELECT p, q, x, y, z, w FROM preview_blocks WHERE preview_id = ?;";
    sqlite3_stmt *stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (rc == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, preview_id);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int p = sqlite3_column_int(stmt, 0);
            int q = sqlite3_column_int(stmt, 1);
            int x = sqlite3_column_int(stmt, 2);
            int y = sqlite3_column_int(stmt, 3);
            int z = sqlite3_column_int(stmt, 4);
            int w = sqlite3_column_int(stmt, 5);

            blocks.emplace_back(p, q, x, y, z, w);
        }

        sqlite3_finalize(stmt);
    }

    load_mtx.unlock();

    return blocks;
}
