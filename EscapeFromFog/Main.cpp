#include <stdexcept>
#include <cstdlib>

#include "EscapeFromFog.hpp"

int main()
{
    EscapeFromFog app;
    try
    {
        app.start();
    }
    catch (std::exception& ex)
    {
        SDL_Log(ex.what());
    }

    return EXIT_SUCCESS;
}

