#ifndef RESOURCELEVEL
#define RESOURCELEVEL

#include <vector>

#include "LevelGenerator.hpp"
#include "ResourceIds.hpp"

namespace ResourceLevels
{
namespace Levels
{
//const std::vector<std::vector<int>> TEST_LEVEL = {{
//    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
//    {0, 3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
//    {0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
//    {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 0, 0},
//    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
//    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
//    {0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0},
//    {0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
//    {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0},
//    {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0},
//    {0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0},
//    {0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0},
//    {0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0},
//    {0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
//    {0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
//    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
//}};

//namespace Tx = ResourceIds::Textures;
using namespace Tile;

const std::vector<std::vector<Data>> TEST_LEVEL = {{
    {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
    {{}, {false,  Special::PLAYER}, {}, {}, {}, {}, {}, {}, {}, {false,Special::NONE}, {}, {}, {}, {}, {}, {}},
    {{}, {false,  Special::NONE}, {false,  Special::SPD_PW}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}},
    {{}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {false,  Special::ENEMY}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {false,  Special::NONE}, {false,  Special::NONE}, {false, Special::ENEMY}, {false,  Special::INVINC_PW}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {}, {}, {false,Special::NONE}, {}, {}, {false,Special::NONE}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {}, {}, {false,Special::NONE}, {}, {}, {false,Special::ENEMY}, {false,Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {false,Special::RCHRG_PW}, {}, {}, {}, {false,  Special::ENEMY}, {}, {}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {false,  Special::NONE}, {}, {false,  Special::ENEMY}, {}, {false,  Special::NONE}, {}, {}},
    {{}, {}, {}, {false,  Special::NONE}, {}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}},
    {{}, {false,  Special::NONE}, {}, {false,  Special::NONE}, {}, {}, {false,  Special::NONE}, {false,  Special::ENEMY}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}},
    {{}, {false,  Special::EXIT}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {false,  Special::NONE}, {false,  Special::NONE}, {}, {}, {}, {}, {}, {}, {}, {}},
    {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}}
}};
} // levels
} // ResourceLevels

#endif // RESOURCELEVEL

