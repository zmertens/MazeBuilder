#include "world.h"

#include <noise/noise.h>

#include "maze_thread_safe.h"

using namespace std;

void world::create_world(int p, int q, const std::unique_ptr<maze_thread_safe>& maze, world_func func, Map* m,
    int chunk_size, bool show_trees, bool show_plants, bool show_clouds) const noexcept {

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
            static constexpr auto PLANT_STARTING_Y = 2;
            for (int y = 0; y < PLANT_STARTING_Y; y++) {
                func(x, y, z, w * flag, m);
            }
            
            if (w == 1) {
                if (show_plants) {
                    // grass
                    if (simplex2(-x * 0.1, z * 0.1, 4, 0.8, 2) > 0.6) {
                        func(x, PLANT_STARTING_Y, z, 17 * flag, m);
                    }
                    // flowers
                    if (simplex2(x * 0.05, -z * 0.05, 4, 0.8, 2) > 0.7) {
                        int w = 18 + simplex2(x * 0.1, z * 0.1, 4, 0.8, 2) * 7;
                        func(x, PLANT_STARTING_Y, z, w * flag, m);
                    }
                }
                // trees
                int ok = show_trees;
                //if (dx - 4 < 0 || dz - 4 < 0 || dx + 4 >= chunk_size || dz + 4 >= chunk_size) {
                //    ok = 0;
                //}
                if (ok && simplex2(x, z, 6, 0.5, 2) > 0.84) {
                    for (int y = PLANT_STARTING_Y + 3; y < PLANT_STARTING_Y + 8; y++) {
                        for (int ox = -3; ox <= 3; ox++) {
                            for (int oz = -3; oz <= 3; oz++) {
                                int d = (ox * ox) + (oz * oz) + (y - (PLANT_STARTING_Y + 4)) * (y - (PLANT_STARTING_Y + 4));
                                if (d < 11) {
                                    func(x + ox, y, z + oz, 15, m);
                                }
                            }
                        }
                    }
                    for (int y = PLANT_STARTING_Y; y < PLANT_STARTING_Y + 7; y++) {
                        func(x, y, z, 5, m);
                    }
                }
            }
            // clouds
            if (show_clouds) {
                for (int y = 64; y < 72; y++) {
                    if (simplex3(x * 0.01, y * 0.1, z * 0.01, 8, 0.5, 2) > 0.75) {
                        func(x, y, z, 16 * flag, m);
                    }
                }
            }
            if (maze == nullptr)
                return;
            const auto& pq = maze->get_p_q();
            bool is_part_of_maze = pq.find({ p, q }) != pq.end();
            // Build the maze
            if (is_part_of_maze) {
                static const unsigned int starting_height = PLANT_STARTING_Y;
                for (auto y = starting_height; y < starting_height + maze->get_height(); y++)
                    func(x, y, z, maze->get_block_type(), m);
            }
        }
    }
} // create_world
