# C-style interface DLL export

Build with `-DMAZE_BUILDER_C_EXPORT=ON` .

Targets:
- `mazebuilder_dll_test`: wraps user arguments and sends to the `maze` function
- `maze.dll`: loadable module

```lua
local ffi = require("ffi")
ffi.cdef[[
  char *maze(const char *args);
  void  maze_free(char *s);
]]

-- ffi.load("maze") finds maze.dll / libmaze.so
local lib = ffi.load("maze")

local function maze(args)
  local p = lib.maze(args)
  if p == nil then return nil, "maze failed" end
  local s = ffi.string(p)
  lib.maze_free(p)
  return s
end

print(maze("--rows=10 --columns=10 --algo=dfs"))
```


`maze(args)` takes CLI-style arguments and returns the result as a string:

```lua
print(maze("--rows=5 --columns=5 --algo=dfs --seed=1"))
```
