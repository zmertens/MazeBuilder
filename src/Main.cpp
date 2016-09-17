#include <stdexcept>
#include <cstdlib>

#include "Shooter.hpp"

int main()
{
    Shooter game;
    try
    {
        game.start();
    }
    catch (std::exception& ex)
    {
        SDL_Log(ex.what());
    }

    return EXIT_SUCCESS;
}

