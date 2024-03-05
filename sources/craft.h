#ifndef CRAFT_H
#define CRAFT_H

#include <string_view>
#include <functional>
#include <memory>
#include <list>

#include <glad/glad.h>

#include <SDL3/SDL.h>

#include <tinycthread/tinycthread.h>

#include "sign.h"
#include "map.h"
#include "config.h"

#include "maze_algo_interface.h"
#include "maze_types_enum.h"

#define MAX_CHUNKS 8192
#define MAX_PLAYERS 128
#define WORKERS 4
#define MAX_TEXT_LENGTH 256
#define MAX_NAME_LENGTH 32
#define MAX_PATH_LENGTH 256
#define MAX_ADDR_LENGTH 256

class grid;
struct Attrib;
struct Player;
struct Block;
struct Chunk;
struct WorkerItem;
struct Worker;

class craft : public mazes::maze_algo_interface {
public:
    craft(const std::string_view& window_name, mazes::maze_types maze_type);
    ~craft();
    // craft(const craft& rhs);
    // craft& operator=(const craft& rhs);
    
    bool run(mazes::grid& gr, std::function<int(int, int)> const& get_int, bool interactive = false) noexcept override;
    std::list<std::unique_ptr<mazes::grid>> get_grids() const noexcept;
private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;

    void convert_grid_to_voxels(const mazes::grid& grid) noexcept;
};

#endif // CRAFT_H
