#include "world.h"

#include <optional>
#include <iostream>

#include <noise/noise.h>

#include "maze_builder.h"

using namespace std;
using namespace mazes;

void world::create_world(int p, int q, const std::unique_ptr<mazes::maze_builder>& maze, world_func func, Map* m, int chunk_size) noexcept {

    int pad = 1;
    for (int dx = -pad; dx < chunk_size + pad; dx++) {
        for (int dz = -pad; dz < chunk_size + pad; dz++) {
            int flag = 1;
            if (dx < 0 || dz < 0 || dx >= chunk_size || dz >= chunk_size) {
                flag = -1;
            }
            int x = p * chunk_size + dx;
            int z = q * chunk_size + dz;

            // Build the environment
            float f = simplex2(static_cast<float>(x) * 0.01, static_cast<float>(z) * 0.01, 4, 0.5, 2);
            float g = simplex2(static_cast<float>(-x) * 0.01, static_cast<float>(-z) * 0.01, 2, 0.9, 2);
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
                func(x, y, z, w * flag, m);
            }

            // Build the maze
            {
				lock_guard<mutex> lock(maze_mutex);
                for (auto y = 1; y < t + maze->get_height(); y++) {
                    const auto& itr = maze->find_block(p, q);
                    if (itr.has_value()) {
                        auto [px, py, pz, block] = itr.value();
                        func(x, y, z, block, m);
                    }
                }
            }

            if (w == 1) {
                // grass
                if (simplex2(-x * 0.1, z * 0.1, 4, 0.8, 2) > 0.6) {
                    func(x, h, z, 17 * flag, m);
                }
                // flowers
                if (simplex2(x * 0.05, -z * 0.05, 4, 0.8, 2) > 0.7) {
                    w = 18 + simplex2(x * 0.1, z * 0.1, 4, 0.8, 2) * 7;
                    func(x, h, z, w * flag, m);
                }
                // trees
                if (simplex2(x, z, 6, 0.5, 2) > 0.84) {
                    for (int y = h + 3; y < h + 8; y++) {
                        for (int ox = -3; ox <= 3; ox++) {
                            for (int oz = -3; oz <= 3; oz++) {
                                int d = (ox * ox) + (oz * oz) + (y - (h + 4)) * (y - (h + 4));
                                if (d < 11) {
                                    func(x + ox, y, z + oz, 15, m);
                                }
                            }
                        }
                    }
                    for (int y = h; y < h + 7; y++) {
                        func(x, y, z, 5, m);
                    }
                }
            }
            // clouds
            for (int y = 64; y < 72; y++) {
                if (simplex3(x * 0.01, y * 0.1, z * 0.01, 8, 0.5, 2) > 0.75) {
                    func(x, y, z, 16 * flag, m);
                }
            }
        }
    }
} // create_world
