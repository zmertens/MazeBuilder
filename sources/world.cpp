#include "config.h"
#include <noise/noise.h>
#include "world.h"

#include <memory>
#include <random>

#include "grid.h"
#include "binary_tree.h"

void create_maze(int p, int q, world_func func, void *arg) {
    mazes::grid grid {5, 5};
    auto get_int = [](int low, int high) -> int {
        using namespace std;
        random_device rd;
        seed_seq seed {rd()};
        mt19937 rng_engine {seed};
        uniform_int_distribution<int> dist {low, high};
        return dist(rng_engine);
    };
    auto bt_ptr {std::make_unique<mazes::binary_tree>()};
    bt_ptr->run(grid, get_int);
    int pad = 1;
    for (auto dx {0}; dx < grid.get_rows() + pad; dx++) {
        for (auto dz{-pad}; dz < grid.get_columns() + pad; dz++) {
            int x = p * grid.get_grid().at(dx).at(dz)->get_row() + dx;
            int z = q * grid.get_grid().at(dx).at(dz)->get_column() + dz;
            int h {10};
            for (int y = 0; y < h; y++) {
                func(x, y, z, 1, arg);
            }
        }
    }
}

void create_world(int p, int q, world_func func, void *arg) {
    int pad = 1;
    for (int dx = -pad; dx < CHUNK_SIZE + pad; dx++) {
        for (int dz = -pad; dz < CHUNK_SIZE + pad; dz++) {
            int flag = 1;
            if (dx < 0 || dz < 0 || dx >= CHUNK_SIZE || dz >= CHUNK_SIZE) {
                flag = -1;
            }
            int x = p * CHUNK_SIZE + dx;
            int z = q * CHUNK_SIZE + dz;
            float f = simplex2(x * 0.01, z * 0.01, 4, 0.5, 2);
            float g = simplex2(-x * 0.01, -z * 0.01, 2, 0.9, 2);
            int mh = g * 32 + 16;
            int h = f * mh;
            int w = 1;
            int t = 12;
            if (h <= t) {
                h = t;
                w = 2;
            }
            // sand and grass terrain
            for (int y = 0; y < h; y++) {
                func(x, y, z, w * flag, arg);
            }
            if (w == 1) {
                if (SHOW_PLANTS) {
                    // grass
                    if (simplex2(-x * 0.1, z * 0.1, 4, 0.8, 2) > 0.6) {
                        func(x, h, z, 17 * flag, arg);
                    }
                    // flowers
                    if (simplex2(x * 0.05, -z * 0.05, 4, 0.8, 2) > 0.7) {
                        int w = 18 + simplex2(x * 0.1, z * 0.1, 4, 0.8, 2) * 7;
                        func(x, h, z, w * flag, arg);
                    }
                }
                // trees
                int ok = SHOW_TREES;
                if (dx - 4 < 0 || dz - 4 < 0 ||
                    dx + 4 >= CHUNK_SIZE || dz + 4 >= CHUNK_SIZE)
                {
                    ok = 0;
                }
                if (ok && simplex2(x, z, 6, 0.5, 2) > 0.84) {
                    for (int y = h + 3; y < h + 8; y++) {
                        for (int ox = -3; ox <= 3; ox++) {
                            for (int oz = -3; oz <= 3; oz++) {
                                int d = (ox * ox) + (oz * oz) +
                                    (y - (h + 4)) * (y - (h + 4));
                                if (d < 11) {
                                    func(x + ox, y, z + oz, 15, arg);
                                }
                            }
                        }
                    }
                    for (int y = h; y < h + 7; y++) {
                        func(x, y, z, 5, arg);
                    }
                }
            }
            // clouds
            if (SHOW_CLOUDS) {
                for (int y = 64; y < 72; y++) {
                    if (simplex3(x * 0.01, y * 0.1, z * 0.01, 8, 0.5, 2) > 0.75) {
                        func(x, y, z, 16 * flag, arg);
                    }
                }
            }
        }
    }
} // create_world
