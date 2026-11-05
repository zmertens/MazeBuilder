#pragma once

struct lua_State;

// Registers the global Lua function maze(args) -> string
void maze_lua_register(lua_State *L);
