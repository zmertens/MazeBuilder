// Simple C-wrapper style function for Lua binding

#pragma once
#ifdef _WIN32
#define MAZE_API __declspec(dllexport)
#else
#define MAZE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    // args: CLI-style string, e.g. "--rows=10 --columns=10 --algo=dfs"
    // Returns a NULL-terminated string (or NULL on failure). Release it with maze_free().
    MAZE_API char *maze(const char *args);
    MAZE_API void maze_free(char *s);

#ifdef __cplusplus
}
#endif
