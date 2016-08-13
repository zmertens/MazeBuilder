#include <stdexcept>
#include <cstdlib>

#include "ShooterGame.hpp"

int main()
{
    ShooterGame app;
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

