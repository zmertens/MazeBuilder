#ifndef WORLD_MANAGER_H
#define WORLD_MANAGER_H

#include "craft_types.h"

#include <vector>

class Block;
class ChunkData;

class WorldManager {
public:
    static void setBlock(int x, int y, int z, int w, std::vector<ChunkData>& chunks, int chunk_size);
    static int getBlock(int x, int y, int z, const std::vector<ChunkData>& chunks, int chunk_size);
    static void recordBlock(int x, int y, int z, int w, Block& block0, Block& block1);
    static void builderBlock(int x, int y, int z, int w, std::vector<ChunkData>& chunks, int chunk_size);
    
    static void setSign(int x, int y, int z, int face, const char* text, 
                       std::vector<ChunkData>& chunks, int chunk_size);
    static void unsetSign(int x, int y, int z, std::vector<ChunkData>& chunks, int chunk_size);
    static void toggleLight(int x, int y, int z, std::vector<ChunkData>& chunks, int chunk_size);
    static void setLight(int p, int q, int x, int y, int z, int w, 
                        std::vector<ChunkData>& chunks);
};

#endif // WORLD_MANAGER_H

