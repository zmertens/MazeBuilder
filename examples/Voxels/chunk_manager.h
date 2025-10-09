#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include "craft_types.h"
#include "gl_types.h"

struct ChunkData {
    int p, q;
    int faces, sign_faces;
    int dirty;
    int miny, maxy;
    gl_unsigned_int buffer, sign_buffer;
    Map map, lights;
    SignList signs;
};

#include <vector>

class ChunkManager {
private:
    std::vector<ChunkData> m_chunks;
    int m_chunk_count;
    int m_create_radius, m_render_radius, m_delete_radius, m_sign_radius;

public:
    ChunkData* findChunk(int p, int q);
    int calculateChunkDistance(const ChunkData* chunk, int p, int q);
    bool isChunkVisible(float planes[6][4], int p, int q, int miny, int maxy, bool is_ortho);
    
    void deleteDistantChunks(float player_x, float player_z, int chunk_size);
    void deleteAllChunks();
    
    void initChunk(ChunkData* chunk, int p, int q, int chunk_size);
    void createChunk(ChunkData* chunk, int p, int q, int chunk_size);
    
    // Getters/Setters
    int getChunkCount() const { return m_chunk_count; }
    const std::vector<ChunkData>& getChunks() const { return m_chunks; }
    void setRadii(int create, int render, int del, int sign);
};

#endif // CHUNK_MANAGER_H
