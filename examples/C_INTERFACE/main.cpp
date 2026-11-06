#include "maze_dll.h"

#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: %s maze(<args>...)\n", argv[0]);
        return EXIT_FAILURE;
    }

    std::string joined;
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
        {
            joined += ' ';
        }
        joined += argv[i];
    }

    if (auto *result{maze(joined.c_str())}; !result)
    {
        std::fprintf(stderr, "maze execution failed\n");
        return EXIT_FAILURE;
    }
    else
    {
        std::printf("%s\n", result);
        maze_free(result);
    }

    return EXIT_SUCCESS;
}
