#include <stdexcept>
#include <cstdlib>

#include "GameImpl.hpp"

int main()
{
    GameImpl blowtorch;
    try
    {
        blowtorch.start();
    }
    catch (std::exception& ex)
    {
        SDL_Log(ex.what());
    }

    return EXIT_SUCCESS;
}

