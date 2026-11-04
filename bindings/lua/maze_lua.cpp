#include "maze_lua.h"

#include <MazeBuilder/runtime_app.h>

#include <memory>
#include <string_view>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}

namespace
{
// maze(args) -> string. args use the CLI syntax, e.g. "--rows=10 --columns=10 --algo=dfs"
int l_maze(lua_State *L)
{
    size_t len = 0;
    const char *args = luaL_checklstring(L, 1, &len);

    auto app = mazes::runtime_app::instance();
    if (!app)
    {
        return luaL_error(L, "maze: runtime app unavailable");
    }

    // The returned view is only valid until the next apply(); copy it into Lua right away
    const std::string_view result = app->apply(std::string_view{args, len});
    lua_pushlstring(L, result.data(), result.size());
    return 1;
}
} // namespace

void maze_lua_register(lua_State *L)
{
    lua_pushcfunction(L, l_maze);
    lua_setglobal(L, "maze");
}

extern "C"
#ifdef _WIN32
    __declspec(dllexport)
#endif
    int luaopen_maze(lua_State *L)
{
    lua_pushcfunction(L, l_maze);
    return 1;
}
