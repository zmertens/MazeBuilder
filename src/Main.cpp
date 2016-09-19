#include <stdexcept>
#include <cstdlib>

#include "Blowtorch.hpp"

int main()
{
    Blowtorch game;
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

