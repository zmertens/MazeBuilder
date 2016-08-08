#include <stdexcept>
#include <cstdlib>

#include "KillDashNine.hpp"

int main()
{
    KillDashNine app;
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

