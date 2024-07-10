#include "world.h"

#include <noise/noise.h>
#include <sstream>

using namespace std;

void world::create_world(int p, int q, world_func func, Map* m, const int CHUNK_SIZE, const bool SHOW_TREES, const bool SHOW_PLANTS, const bool SHOW_CLOUDS,
    unsigned int height, std::istringstream maze) const noexcept {
    unsigned int row_x = 0;
    string line = "";

    int pad = 1;
    for (int dx = -pad; dx < CHUNK_SIZE + pad; dx++) {
        for (int dz = -pad; dz < CHUNK_SIZE + pad; dz++) {
            int flag = 1;
            if (dx < 0 || dz < 0 || dx >= CHUNK_SIZE || dz >= CHUNK_SIZE) {
                flag = -1;
            }
            int x = p * CHUNK_SIZE + dx;
            int z = q * CHUNK_SIZE + dz;

            // Reset the stringstream for each new chunk
            //iss.clear();
            //iss.seekg(0, ios::beg);

            unsigned int row_x = 0;
            while (getline(maze, line)) {
                unsigned int col_z = 0;
                for (char c : line) {
                    if (c == ' ') {
                        col_z++;
                        continue;
                    } else if (c == '+' || c == '-' || c == '|') {
                        // Check for barriers and walls then iterate up/down
                        static constexpr unsigned int starting_height = 30u;
                        for (unsigned int h = starting_height; h < starting_height + height; h++) {
                            // Set the block in the craft
                            func(x + row_x, h, z + col_z, 6, m);
                        }
                        col_z++;
                    }
                }
                row_x++;
            }
        }
    }
}
            //float f = simplex2(static_cast<float>(x) * 0.01, static_cast<float>(z) * 0.01, 4, 0.5, 2);
            //float g = simplex2(static_cast<float>(-x) * 0.01, static_cast<float>(-z) * 0.01, 2, 0.9, 2);
            //int mh = g * 32 + 16;
            //int h = f * mh;
            //int w = 1;
            //int t = 12;
            //if (h <= t) {
            //    h = t;
            //    w = 2;
            //}
            //// sand and grass terrain
            //for (int y = 0; y < h; y++) {
            //    func(x, y, z, w * flag, m);
            //}
            //if (w == 1) {
            //    if (SHOW_PLANTS) {
            //        // grass
            //        if (simplex2(-x * 0.1, z * 0.1, 4, 0.8, 2) > 0.6) {
            //            func(x, h, z, 17 * flag, m);
            //        }
            //        // flowers
            //        if (simplex2(x * 0.05, -z * 0.05, 4, 0.8, 2) > 0.7) {
            //            int w = 18 + simplex2(x * 0.1, z * 0.1, 4, 0.8, 2) * 7;
            //            func(x, h, z, w * flag, m);
            //        }
            //    }
            //    // trees
            //    int ok = SHOW_TREES;
            //    if (dx - 4 < 0 || dz - 4 < 0 ||
            //        dx + 4 >= CHUNK_SIZE || dz + 4 >= CHUNK_SIZE)
            //    {
            //        ok = 0;
            //    }
            //    if (ok && simplex2(x, z, 6, 0.5, 2) > 0.84) {
            //        for (int y = h + 3; y < h + 8; y++) {
            //            for (int ox = -3; ox <= 3; ox++) {
            //                for (int oz = -3; oz <= 3; oz++) {
            //                    int d = (ox * ox) + (oz * oz) +
            //                        (y - (h + 4)) * (y - (h + 4));
            //                    if (d < 11) {
            //                        func(x + ox, y, z + oz, 15, m);
            //                    }
            //                }
            //            }
            //        }
            //        for (int y = h; y < h + 7; y++) {
            //            func(x, y, z, 5, m);
            //        }
            //    }
            //}
            //// clouds
            //if (SHOW_CLOUDS) {
            //    for (int y = 64; y < 72; y++) {
            //        if (simplex3(x * 0.01, y * 0.1, z * 0.01, 8, 0.5, 2) > 0.75) {
            //            func(x, y, z, 16 * flag, m);
            //        }
            //    }
            //}

            //for (int i = 0; i < 100; i++) {
            //    static constexpr auto height = 35;
            //    func(x, height, z, 6, m);
            //}




    //istringstream iss(maze);
    //string line = "";
    //unsigned int row_x = 0;
    //while (getline(iss, line, '\n')) {
    //    unsigned int col_z = 0;
    //    for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {
    //        if (*itr == ' ') {
    //            col_z++;
    //        } else if (*itr == '+' || *itr == '-' || *itr == '|') {
    //            // check for barriers and walls then iterate up/down
    //            static constexpr unsigned int starting_height = 30u, height = 5u;
    //            static constexpr float block_size = 1.0f;
    //            for (auto h{ starting_height }; h < starting_height + height; h++) {
    //                // set the block in the craft
    //                func(row_x, h, col_z, 6, m);
    //            }
    //            col_z++;
    //        }
    //    }

    //    row_x++;
    //} // getline
//} // create_world

void world::create_maze(int p, int q, int w, unsigned int height, world_func func, Map* m, const int CHUNK_SIZE, std::istringstream iss) const noexcept {
    string line = "";
    unsigned int row_x = 0;
    while (getline(iss, line, '\n')) {
        unsigned int col_z = 0;
        for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {
            if (*itr == ' ') {
                col_z++;
            } else if (*itr == '+' || *itr == '-' || *itr == '|') {
                // check for barriers and walls then iterate up/down
                static constexpr unsigned int starting_height = 30u;
                static constexpr float block_size = 1.0f;
                for (auto h{ starting_height }; h < starting_height + height; h++) {
                    // set the block in the craft
                    func(row_x, h, col_z, w, m);
                }
                col_z++;
            }
        }

        row_x++;
    } // getline
}
