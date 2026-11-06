// Simple C-wrapper style function for Lua binding

#include "maze_dll.h"

#include <MazeBuilder/runtime_app.h>

#include <cstdlib>
#include <memory>
#include <cstring>
#include <string>

char *maze(const char *args)
{
    if (!args)
    {
        return nullptr;
    }

    if (auto app = mazes::runtime_app::instance(); !app)
    {
        return nullptr;
    }
    else
    {
        const std::string result{app->apply(std::string_view{args})};
        char *out = static_cast<char *>(std::malloc(result.size() + 1));
        if (!out)
        {
            return nullptr;
        }
        std::memcpy(out, result.c_str(), result.size() + 1);
        return out;
    }
}

void maze_free(char *s) { std::free(s); }
