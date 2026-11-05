#include "maze_lua.h"

#include <cstdio>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: %s script.lua\n", argv[0]);
        return 2;
    }

    lua_State *L = luaL_newstate();
    if (!L)
    {
        std::fprintf(stderr, "failed to create Lua state\n");
        return 1;
    }
    luaL_openlibs(L);
    maze_lua_register(L);

    int rc = 0;
    if (luaL_dofile(L, argv[1]) != LUA_OK)
    {
        std::fprintf(stderr, "%s\n", lua_tostring(L, -1));
        rc = 1;
    }
    lua_close(L);
    return rc;
}
