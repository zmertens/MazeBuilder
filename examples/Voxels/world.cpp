#include "world.h"

#include <noise/noise.h>

using namespace std;
using namespace mazes;

void world::create_world(int p, int q, world_func func, Map* m, int chunk_size, const vector<unique_ptr<maze>>& my_mazes) const noexcept {

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

            static constexpr auto PLANT_HEIGHT_MAX = 2;

            // Maze
            for (const auto& my_maze : my_mazes) {
                const auto& block = my_maze->find_block(x, z);
                if (block.has_value()) {
                    const auto& [r, height, c, t] = block.value();
                    for (auto y = 0; y < height + PLANT_HEIGHT_MAX + 1; y++) {
                        func(r, y, c, t * flag, m);
                    }
                    continue;
                }
            }

            // sand and grass terrain            
            for (int y = 0; y < PLANT_HEIGHT_MAX; y++) {
                func(x, y, z, w * flag, m);
            }
            
            if (w == 1) {
                // grass
                if (simplex2(-x * 0.1, z * 0.1, 4, 0.8, 2) > 0.6) {
                    func(x, PLANT_HEIGHT_MAX, z, 17 * flag, m);
                }
                // flowers
                if (simplex2(x * 0.05, -z * 0.05, 4, 0.8, 2) > 0.7) {
                    int w = 18 + simplex2(x * 0.1, z * 0.1, 4, 0.8, 2) * 7;
                    func(x, PLANT_HEIGHT_MAX, z, w * flag, m);
                }
                // trees
                // Check that the tree fits in the chunk
                bool ok = true;
                if (dx - 3 < 0 || dz - 3 < 0 || dx + 4 > chunk_size || dz + 4 > chunk_size) {
                   ok = false;
                }
                if (ok && simplex2(x, z, 6, 0.5, 2) > 0.84) {
                    // Generate canopy for tree (leaves)
                    for (int y = PLANT_HEIGHT_MAX + 3; y < PLANT_HEIGHT_MAX + 8; y++) {
                        for (int ox = -3; ox <= 3; ox++) {
                            for (int oz = -3; oz <= 3; oz++) {
                                int d = (ox * ox) + (oz * oz) + (y - (PLANT_HEIGHT_MAX + 4)) * (y - (PLANT_HEIGHT_MAX + 4));
                                if (d < 11) {
                                    func(x + ox, y, z + oz, 15, m);
                                }
                            }
                        }
                    }
                    // Generate the tree trunk
                    for (int y = PLANT_HEIGHT_MAX; y < PLANT_HEIGHT_MAX + 7; y++) {
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
