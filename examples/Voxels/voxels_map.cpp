#include "voxels_map.h"

// ── Integer hashing (unchanged from original) ─────────────────────────────
static int hash_int(int key) noexcept
{
    key = ~key + (key << 15);
    key = key ^ (key >> 12);
    key = key + (key << 2);
    key = key ^ (key >> 4);
    key = key * 2057;
    key = key ^ (key >> 16);
    return key;
}

static int hash3(const int x, const int y, const int z) noexcept
{
    return hash_int(x) ^ hash_int(y) ^ hash_int(z);
}

static bool is_empty(const MapEntry &entry) noexcept
{
    return entry.value == 0;
}

static bool is_occupied(const MapEntry &entry) noexcept
{
    return entry.e.w != 0;
}

// ── iterator ──────────────────────────────────────────────────────────────
voxels_map::voxel voxels_map::iterator::operator*() const noexcept
{
    const MapEntry &e = m->m_data[idx];
    return {e.e.x + m->m_dx, e.e.y + m->m_dy, e.e.z + m->m_dz, e.e.w};
}

voxels_map::iterator &voxels_map::iterator::operator++() noexcept
{
    do
    {
        ++idx;
    } while (idx <= m->m_mask && !is_occupied(m->m_data[idx]));
    return *this;
}

// ── voxels_map ───────────────────────────────────────────────────────────────────────────
voxels_map::voxels_map(const int dx, const int dy, const int dz, const unsigned int mask)
{
    init(dx, dy, dz, mask);
}

void voxels_map::init(const int dx, const int dy, const int dz, const unsigned int mask)
{
    m_dx = dx;
    m_dy = dy;
    m_dz = dz;
    m_mask = mask;
    m_size = 0;
    m_data.assign(mask + 1u, MapEntry{}); // zero-initialise all slots
}

voxels_map::iterator voxels_map::begin() const noexcept
{
    unsigned int i = 0;
    while (i <= m_mask && !is_occupied(m_data[i]))
    {
        ++i;
    }
    return {this, i};
}

voxels_map::iterator voxels_map::end() const noexcept
{
    return {this, m_mask + 1u};
}

int voxels_map::set(int x, int y, int z, const int w)
{
    if (m_data.empty())
    {
        return 0;
    }

    const int abs_x = x;
    const int abs_y = y;
    const int abs_z = z;

    x -= m_dx;
    y -= m_dy;
    z -= m_dz;

    if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255)
    {
        return 0;
    }

    auto index = static_cast<unsigned int>(hash3(abs_x, abs_y, abs_z)) & m_mask;
    unsigned int first_tombstone = m_mask + 1u;

    for (unsigned int probe = 0; probe <= m_mask; ++probe)
    {
        MapEntry *entry = m_data.data() + index;

        if (is_empty(*entry))
        {
            if (w == 0)
            {
                return 0;
            }

            if (first_tombstone <= m_mask)
            {
                entry = m_data.data() + first_tombstone;
            }

            entry->e.x = static_cast<unsigned char>(x);
            entry->e.y = static_cast<unsigned char>(y);
            entry->e.z = static_cast<unsigned char>(z);
            entry->e.w = static_cast<char>(w);

            ++m_size;
            if (m_size * 2 > m_mask)
            {
                grow();
            }
            return 1;
        }

        if (!is_occupied(*entry))
        {
            if (first_tombstone > m_mask)
            {
                first_tombstone = index;
            }
        }
        else if (entry->e.x == x && entry->e.y == y && entry->e.z == z)
        {
            if (w == 0)
            {
                entry->e.w = 0;
                --m_size;
                return 1;
            }

            if (entry->e.w != static_cast<char>(w))
            {
                entry->e.w = static_cast<char>(w);
                return 1;
            }
            return 0;
        }

        index = (index + 1) & m_mask;
    }

    if (w != 0 && first_tombstone <= m_mask)
    {
        MapEntry *entry = m_data.data() + first_tombstone;
        entry->e.x = static_cast<unsigned char>(x);
        entry->e.y = static_cast<unsigned char>(y);
        entry->e.z = static_cast<unsigned char>(z);
        entry->e.w = static_cast<char>(w);

        ++m_size;
        if (m_size * 2 > m_mask)
        {
            grow();
        }
        return 1;
    }

    if (w != 0)
    {
        grow();
        return set(abs_x, abs_y, abs_z, w);
    }

    return 0;
}

int voxels_map::get(int x, int y, int z) const noexcept
{
    if (m_data.empty())
    {
        return 0;
    }
    auto index = static_cast<unsigned int>(hash3(x, y, z)) & m_mask;
    x -= m_dx;
    y -= m_dy;
    z -= m_dz;
    if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255)
    {
        return 0;
    }
    for (unsigned int probe = 0; probe <= m_mask; ++probe)
    {
        const MapEntry *entry = m_data.data() + index;
        if (is_empty(*entry))
        {
            return 0;
        }
        if (is_occupied(*entry) && entry->e.x == x && entry->e.y == y && entry->e.z == z)
        {
            return entry->e.w;
        }
        index = (index + 1) & m_mask;
    }
    return 0;
}

void voxels_map::grow()
{
    voxels_map grown{m_dx, m_dy, m_dz, (m_mask << 1u) | 1u};
    for (const auto [ex, ey, ez, ew] : *this)
    {
        grown.set(ex, ey, ez, ew);
    }

    *this = std::move(grown);
}
