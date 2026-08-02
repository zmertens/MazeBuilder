#ifndef VOXELS_MAP_H
#define VOXELS_MAP_H

#include <cstdint>
#include <vector>

// Compact 4-byte entry: value == 0 means the slot is empty.
union MapEntry
{
    unsigned int value;
    struct
    {
        unsigned char x;
        unsigned char y;
        unsigned char z;
        char w;
    } e;
};

// 3-D integer → int sparse voxel map.
//
// Implemented as an open-addressing hash table with linear probing.
// Keys are stored as unsigned-byte offsets from a (dx, dy, dz) base origin,
// so valid absolute coordinates must satisfy  0 ≤ (coord − base) ≤ 255.
//
// Supports range-for via a nested iterator that yields decoded absolute
// coordinates, replacing the old MAP_FOR_EACH / END_MAP_FOR_EACH macros.
class voxels_map
{
public:
    // Value exposed by the iterator.
    struct voxel
    {
        int x, y, z, w;
    };

    // Forward iterator over occupied (non-zero) entries.
    struct iterator
    {
        const voxels_map *m;
        unsigned int idx;
        voxel operator*() const noexcept;
        iterator &operator++() noexcept;
        bool operator!=(const iterator &o) const noexcept { return idx != o.idx; }
    };

    voxels_map() = default;
    // Construct and initialise with explicit base offsets and table size.
    // mask must be (2^n − 1).
    voxels_map(int dx, int dy, int dz, unsigned int mask);

    // Rule-of-five: std::vector provides automatic deep-copy and destruction.
    voxels_map(const voxels_map &) = default;
    voxels_map &operator=(const voxels_map &) = default;
    voxels_map(voxels_map &&) noexcept = default;
    voxels_map &operator=(voxels_map &&) noexcept = default;
    ~voxels_map() = default;

    // Re-initialise in place (replaces the old map_alloc pattern on existing objects).
    void init(int dx, int dy, int dz, unsigned int mask);

    // Write a voxel. Returns 1 if the map changed, 0 otherwise.
    int set(int x, int y, int z, int w);

    // Read a voxel.  Returns 0 for out-of-range or absent coordinates.
    [[nodiscard]] int get(int x, int y, int z) const noexcept;

    // Range-for support.
    [[nodiscard]] iterator begin() const noexcept;
    [[nodiscard]] iterator end() const noexcept;

    [[nodiscard]] unsigned int size() const noexcept { return m_size; }
    [[nodiscard]] bool empty() const noexcept { return m_size == 0; }
    [[nodiscard]] int dx() const noexcept { return m_dx; }
    [[nodiscard]] int dy() const noexcept { return m_dy; }
    [[nodiscard]] int dz() const noexcept { return m_dz; }

private:
    void grow();

    int m_dx{};
    int m_dy{};
    int m_dz{};
    unsigned int m_mask{};
    unsigned int m_size{};
    std::vector<MapEntry> m_data;
};

#endif // VOXELS_MAP_H