#include "world.h"

#include <noise/noise.h>
#include <sstream>
#include <iostream>

using namespace std;

/**
 * @brief The world ultimately rendered on screen
 */
void world::create_world(int p, int q, world_func func, Map* m, const int CHUNK_SIZE, const bool SHOW_TREES, const bool SHOW_PLANTS, const bool SHOW_CLOUDS,
    unsigned int height, 
    const std::vector<std::tuple<std::string::iterator, string::iterator, int>>& maze_parts) const noexcept {
    // Handle null func or m
    if (!func || !m) {
        return;
    }


    static constexpr int MAZE_START_X = 0, MAZE_START_Z = 0;

    int pad = 1;
    for (int dx = -pad; dx < CHUNK_SIZE + pad; dx++) {
        for (int dz = -pad; dz < CHUNK_SIZE + pad; dz++) {
            // Flag the block if the worker is outside the chunk
            int flag = 1;
            if (dx < 0 || dz < 0 || dx >= CHUNK_SIZE || dz >= CHUNK_SIZE) {
                flag = -1;
            }
            int world_x = p * CHUNK_SIZE + dx;
            int world_z = q * CHUNK_SIZE + dz;

            int maze_offset_x = (world_x - MAZE_START_X) % CHUNK_SIZE;
            int maze_offset_z = (world_z - MAZE_START_Z) % CHUNK_SIZE;
            // Ensure positive offsets
            if (maze_offset_x < 0) {
                maze_offset_x += CHUNK_SIZE;
            }
            if (maze_offset_z < 0) {
                maze_offset_z += CHUNK_SIZE;
            }

            // Draw the condensed maze within each chunk
            for (auto&& maze_part : maze_parts) {
                auto begin_itr = get<0>(maze_part);
                auto end_itr = get<1>(maze_part);

                if (begin_itr == end_itr) {
                    continue;
                }

                int start_from_this_row = get<2>(maze_part);
                int offset = start_from_this_row;// * CHUNK_SIZE;
                int z = (maze_offset_x + offset) % CHUNK_SIZE;
                unsigned int col_z = 0;
                for (auto itr = begin_itr; itr != end_itr; ++itr) {
                    char c = *itr;
                    // Draw walls here
                    if (c == '+' || c == '|' || c == '-') {
                        static constexpr unsigned int starting_height = 32u;
                        for (auto h = starting_height; h < starting_height + height; h++) {
                            func((maze_offset_x + col_z) % CHUNK_SIZE, h, maze_offset_z, 6 * flag, m);
                        }
                    }
                }
                // increment the column after every char
                col_z = (col_z + 1) % CHUNK_SIZE;
            }

            // float f = simplex2(static_cast<float>(x) * 0.01, static_cast<float>(base_z) * 0.01, 4, 0.5, 2);
            // float g = simplex2(static_cast<float>(-x) * 0.01, static_cast<float>(-base_z) * 0.01, 2, 0.9, 2);
            // int mh = g * 32 + 16;
            // int h = f * mh;
            // int w = 1;
            // int t = 12;
            // if (h <= t) {
            //     h = t;
            //     w = 2;
            // }
            // // sand and grass terrain
            // for (int y = 0; y < h; y++) {
            //     func(x, y, base_z, w * flag, m);
            // }
            // if (w == 1) {
            //     if (SHOW_PLANTS) {
            //         // grass
            //         if (simplex2(-x * 0.1, base_z * 0.1, 4, 0.8, 2) > 0.6) {
            //             func(x, h, base_z, 17 * flag, m);
            //         }
            //         // flowers
            //         if (simplex2(x * 0.05, -base_z * 0.05, 4, 0.8, 2) > 0.7) {
            //             int w = 18 + simplex2(x * 0.1, base_z * 0.1, 4, 0.8, 2) * 7;
            //             func(x, h, base_z, w * flag, m);
            //         }
            //     }
            //     // trees
            //     int ok = SHOW_TREES;
            //     if (dx - 4 < 0 || dz - 4 < 0 || dx + 4 >= CHUNK_SIZE || dz + 4 >= CHUNK_SIZE) {
            //         ok = 0;
            //     }
            //     if (ok && simplex2(x, base_z, 6, 0.5, 2) > 0.84) {
            //         for (int y = h + 3; y < h + 8; y++) {
            //             for (int ox = -3; ox <= 3; ox++) {
            //                 for (int oz = -3; oz <= 3; oz++) {
            //                     int d = (ox * ox) + (oz * oz) +
            //                         (y - (h + 4)) * (y - (h + 4));
            //                     if (d < 11) {
            //                         func(x + ox, y, base_z + oz, 15, m);
            //                     }
            //                 }
            //             }
            //         }
            //         for (int y = h; y < h + 7; y++) {
            //             func(x, y, base_z, 5, m);
            //         }
            //     }
            // }
            // if (SHOW_CLOUDS) {
            //     for (int y = 64; y < 72; y++) {
            //         if (simplex3(x * 0.01, y * 0.1, base_z * 0.01, 8, 0.5, 2) > 0.75) {
            //             func(x, y, base_z, 16 * flag, m);
            //         }
            //     }
            // }

            // for (int i = 0; i < 100; i++) {
            //     static constexpr auto height = 35;
            //     func(x, height, base_z, 6, m);
            // }

        }
    }
} // create_world
