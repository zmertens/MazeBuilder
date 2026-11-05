# Lua binding

Build with `-DMAZE_BUILDER_LUA_BINDING=ON` (vanilla Lua 5.4 is fetched via FetchContent).

Targets:
- `mazebuilder_lua`: interpreter with a global `maze` function: `mazebuilder_lua script.lua`
- `maze` (loadable module): `local maze = require("maze")`

`maze(args)` takes CLI-style arguments and returns the result as a string:

```lua
print(maze("--rows=5 --columns=5 --algo=dfs --seed=1"))
```
