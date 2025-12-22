#include "ring.h"

#include <string>

#include <SDL3/SDL.h>


void ring_alloc(Ring *ring, const int capacity) {
    ring->capacity = capacity;
    ring->start = 0;
    ring->end = 0;
    ring->data = static_cast<RingEntry*>(SDL_calloc(capacity, sizeof(RingEntry)));
}

void ring_free(const Ring *ring) {
    SDL_free(ring->data);
}

int ring_empty(const Ring *ring) {
    return ring->start == ring->end;
}

int ring_full(const Ring *ring) {
    return ring->start == (ring->end + 1) % ring->capacity;
}

int ring_size(const Ring *ring) {
    if (ring->end >= ring->start) {
        return ring->end - ring->start;
    }
    return ring->capacity - (ring->start - ring->end);
}

void ring_grow(Ring *ring) {
    Ring new_ring;
    RingEntry entry;
    ring_alloc(&new_ring, ring->capacity * 2);
    while (ring_get(ring, &entry)) {
        ring_put(&new_ring, &entry);
    }
    SDL_free(ring->data);
    ring->capacity = new_ring.capacity;
    ring->start = new_ring.start;
    ring->end = new_ring.end;
    ring->data = new_ring.data;
}

void ring_put(Ring *ring, const RingEntry *entry) {
    if (ring_full(ring)) {
        ring_grow(ring);
    }
    RingEntry *e = ring->data + ring->end;
    SDL_memcpy(e, entry, sizeof(RingEntry));
    ring->end = (ring->end + 1) % ring->capacity;
}

void ring_put_block(Ring *ring, const int p, const int q, const int x, const int y, const int z, const int w) {
    RingEntry entry;
    entry.type = RingEntryType::BLOCK;
    entry.p = p;
    entry.q = q;
    entry.x = x;
    entry.y = y;
    entry.z = z;
    entry.w = w;
    entry.blocks = nullptr;
    ring_put(ring, &entry);
}

void ring_put_blocks(Ring* ring, int *blocks) {
    RingEntry entry;
    entry.type = RingEntryType::BLOCKS;
    entry.p = 0;
    entry.q = 0;
    entry.x = 0;
    entry.y = 0;
    entry.z = 0;
    entry.w = 0;
    entry.blocks = blocks;
    ring_put(ring, &entry);
}

void ring_put_light(Ring *ring, const int p, const int q, const int x, const int y, const int z, const int w) {
    RingEntry entry;
    entry.type = RingEntryType::LIGHT;
    entry.p = p;
    entry.q = q;
    entry.x = x;
    entry.y = y;
    entry.z = z;
    entry.w = w;
    ring_put(ring, &entry);
}

void ring_put_key(Ring *ring, const int p, const int q, const int key) {
    RingEntry entry;
    entry.type = RingEntryType::KEY;
    entry.p = p;
    entry.q = q;
    entry.key = key;
    ring_put(ring, &entry);
}

void ring_put_commit(Ring *ring) {
    RingEntry entry;
    entry.type = RingEntryType::COMMIT;
    ring_put(ring, &entry);
}

void ring_put_exit(Ring *ring) {
    RingEntry entry;
    entry.type = RingEntryType::EXIT;
    ring_put(ring, &entry);
}

int ring_get(Ring *ring, RingEntry *entry) {
    if (ring_empty(ring)) {
        return 0;
    }
    const RingEntry *e = ring->data + ring->start;
    SDL_memcpy(entry, e, sizeof(RingEntry));
    ring->start = (ring->start + 1) % ring->capacity;
    return 1;
}
