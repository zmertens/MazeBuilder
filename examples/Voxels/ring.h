#ifndef RING_H
#define RING_H

enum class RingEntryType {
    BLOCK,
    // Part of a vector
    BLOCKS,
    LIGHT,
    KEY,
    COMMIT,
    EXIT
};

struct RingEntry {
    RingEntryType type;
    int p;
    int q;
    int x;
    int y;
    int z;
    int w;
    int *blocks;
    int key;
};

struct Ring {
    unsigned int capacity;
    unsigned int start;
    unsigned int end;
    RingEntry *data;
};

void ring_alloc(Ring *ring, int capacity);
void ring_free(const Ring *ring);
int ring_empty(const Ring *ring);
int ring_full(const Ring *ring);
int ring_size(const Ring *ring);
void ring_grow(Ring *ring);
void ring_put(Ring *ring, const RingEntry *entry);
void ring_put_block(Ring *ring, int p, int q, int x, int y, int z, int w);
void ring_put_blocks(Ring* ring, int* blocks);
void ring_put_light(Ring *ring, int p, int q, int x, int y, int z, int w);
void ring_put_key(Ring *ring, int p, int q, int key);
void ring_put_commit(Ring *ring);
void ring_put_exit(Ring *ring);
int ring_get(Ring *ring, RingEntry *entry);

#endif // RING_H
